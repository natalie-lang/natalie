module Natalie
  class Compiler
    class LlvmBackend
      # Shared LLVM emission state for instruction generate_llvm methods.
      # Each allocation happens in the entry block, including argument arrays and
      # conservative GC roots, so loops use a bounded amount of native stack.
      class IR
        DECLARATIONS = <<~LLVM.freeze
          declare i64 @nat_llvm_integer(ptr)
          declare i64 @nat_llvm_float(double)
          declare i64 @nat_llvm_string(ptr, i64, i32)
          declare i64 @nat_llvm_symbol(ptr, i64)
          declare i64 @nat_llvm_array(i64, ptr)
          declare i64 @nat_llvm_array_shift(i64)
          declare i64 @nat_llvm_array_pop(i64)
          declare i64 @nat_llvm_object_class()
          declare i64 @nat_llvm_const(ptr, i64, i64, ptr, i32, i32)
          declare i64 @nat_llvm_send(ptr, i64, i64, ptr, i64, ptr, i32, i32, ptr, i64, i64)
          declare void @nat_llvm_build_vars(ptr, i64)
          declare i64 @nat_llvm_var_get(ptr, i64, i64)
          declare void @nat_llvm_var_set(ptr, i64, i64, i64)
          declare i64 @nat_llvm_block(ptr, i64, ptr, i32)
          declare void @nat_llvm_define_method(ptr, i64, ptr, ptr, i32, ptr, i64)
          declare i64 @nat_llvm_arg(ptr, i64)
          declare void @nat_llvm_check_args(ptr, ptr, i64, i64)
          declare i64 @nat_llvm_args(ptr, ptr, i32, i64, i64, i32, i32)
          declare i64 @nat_llvm_yield(ptr, i64, ptr, i32)
          declare i64 @nat_llvm_global_get(ptr, ptr)
          declare void @nat_llvm_global_set(ptr, ptr, i64)
          declare i64 @nat_llvm_hash(ptr, i64, ptr)
          declare i64 @nat_llvm_range(ptr, i64, i64, i32)
          declare i64 @nat_llvm_ivar_get(ptr, i64, ptr)
          declare void @nat_llvm_ivar_set(ptr, i64, ptr, i64)
          declare void @nat_llvm_const_set(ptr, i64, ptr, i64, i32)
          declare i64 @nat_llvm_define_type(ptr, i64, i64, ptr, i64, ptr, i32, i32)
          declare i64 @nat_llvm_load(ptr, ptr, ptr, i32)
          declare i64 @nat_llvm_args_empty(ptr, ptr, i32)
          declare i64 @nat_llvm_args_shift(ptr, ptr, i32)
          declare i64 @nat_llvm_args_pop(ptr, ptr, i32)
          declare i64 @nat_llvm_lambda(ptr, i64)
          declare i64 @nat_llvm_push_block(ptr, ptr, i32)
          declare void @nat_llvm_set_zero(ptr, ptr)
          declare void @nat_llvm_warn(ptr, i64, ptr, i32)
        LLVM

        def initialize(
          instructions,
          source_path:,
          shared: nil,
          function_name: 'nat_llvm_eval',
          scope: nil,
          required_files: {}
        )
          @instructions = instructions
          @source_path = source_path
          @stack = []
          @shared = shared || { globals: [], functions: [], serial: 0 }
          @globals = @shared[:globals]
          @function_name = function_name
          @scope = scope
          @required_files = required_files
          @allocas = []
          @body = []
        end

        def generate
          if @function_name == 'nat_llvm_eval'
            emit "call void @nat_llvm_set_zero(ptr %env, ptr #{string(@source_path)})"
          end
          emit "call void @nat_llvm_build_vars(ptr %env, i64 #{@scope ? @scope.fetch(:vars).size : top_scope.fetch(:vars).size})"
          transform(@instructions)
          emit "ret i64 #{@stack.last || 4}"
          function = <<~LLVM
            define i64 @#{@function_name}(ptr %env, i64 %self#{@function_name == 'nat_llvm_eval' ? '' : ', ptr %args, ptr %block'}) uwtable {
            entry:
            #{@allocas.join("\n")}
            #{@body.join("\n")}
            }
          LLVM
          return function unless @function_name == 'nat_llvm_eval'
          <<~LLVM
            ; Natalie experimental LLVM backend (64-bit hosts)
            #{@globals.join("\n")}
            #{DECLARATIONS}
            #{@shared[:functions].join("\n")}
            #{function}
          LLVM
        end

        def function_block
          @function_name == 'nat_llvm_eval' ? 'null' : '%block'
        end

        def top_scope
          env = @instructions.to_a.first&.env
          return { vars: {} } unless env
          env = env[:outer] while env[:outer]
          env
        end

        def function(body, scope:)
          fn = name('fn')
          @shared[:functions] << self
            .class
            .new(
              body,
              source_path: @source_path,
              shared: @shared,
              function_name: fn,
              scope: scope,
              required_files: @required_files,
            )
            .generate
          "@#{fn}"
        end

        def loaded_file(path)
          @shared[:loaded_files] ||= {}
          return @shared[:loaded_files][path] if @shared[:loaded_files].key?(path)
          file = @required_files.fetch(path)
          fn = name('loaded')
          @shared[:loaded_files][path] = "@#{fn}"
          @shared[:functions] << self
            .class
            .new(
              file.instructions,
              source_path: path,
              shared: @shared,
              function_name: fn,
              required_files: @required_files,
            )
            .generate
          "@#{fn}"
        end

        # Pass2 already resolves each local to its owning scope. Count Ruby
        # scopes here; the bridge skips its callback environment at each hop.
        def variable_address(instruction)
          env = instruction.env
          env = env[:outer] while env[:hoist]
          depth = 0
          until env[:vars].values.any? { |var| var.equal?(instruction.meta) }
            env = env.fetch(:outer)
            env = env[:outer] while env[:hoist]
            depth += 1
          end
          [depth, instruction.meta.fetch(:index)]
        end

        attr_reader :stack, :source_path

        def transform(instructions)
          previous = @current_instructions
          @current_instructions =
            instructions.is_a?(InstructionManager) ? instructions : InstructionManager.new(instructions)
          @current_instructions.walk { |instruction| instruction.generate_llvm(self) }
        ensure
          @current_instructions = previous
        end

        def fetch_block_of_instructions(**options)
          @current_instructions.fetch_block(**options)
        end

        def push(operand)
          @stack << operand
        end

        def peek
          @stack.last || raise(CompileError, 'LLVM: instruction stack underflow')
        end

        def argument_array(args)
          return 'null' if args.empty?
          array = "%#{name('args')}"
          @allocas << "  #{array} = alloca [#{args.size} x i64], align 8"
          args.each_with_index do |arg, index|
            ptr = value("getelementptr [#{args.size} x i64], ptr #{array}, i64 0, i64 #{index}")
            emit "store i64 #{arg}, ptr #{ptr}"
          end
          array
        end

        def truthy(operand)
          non_nil = value("icmp ne i64 #{operand}, 4")
          non_false = value("icmp ne i64 #{operand}, 0")
          value("and i1 #{non_nil}, #{non_false}")
        end

        def push_call(call)
          result = value(call)
          # Volatile stack slots keep objects visible to the conservative collector,
          # including values LLVM would otherwise keep only in registers.
          root = slot('root')
          emit "store volatile i64 #{result}, ptr #{root}"
          @stack << result
        end

        def slot(prefix)
          ptr = "%#{name(prefix)}"
          @allocas << "  #{ptr} = alloca i64, align 8"
          @allocas << "  store volatile i64 4, ptr #{ptr}"
          ptr
        end

        def string(str)
          bytes = str.b.bytes + [0]
          global = "@#{name('str')}"
          escaped = bytes.map { |byte| '\\' + byte.to_s(16).rjust(2, '0') }.join
          @globals << "#{global} = private unnamed_addr constant [#{bytes.size} x i8] c\"#{escaped}\""
          global
        end

        def value(expression)
          var = "%#{name('v')}"
          emit "#{var} = #{expression}"
          var
        end

        def name(prefix)
          @shared[:serial] += 1
          "#{prefix}#{@shared[:serial]}"
        end

        def pop(count = nil)
          if count
            raise CompileError, 'LLVM: instruction stack underflow' if @stack.size < count
            @stack.pop(count)
          else
            @stack.pop || raise(CompileError, 'LLVM: instruction stack underflow')
          end
        end

        def emit(line)
          @body << "  #{line}"
        end

        def unsupported(instruction, reason = nil)
          raise CompileError, "LLVM backend does not support #{instruction}#{": #{reason}" if reason}"
        end
      end
    end
  end
end
