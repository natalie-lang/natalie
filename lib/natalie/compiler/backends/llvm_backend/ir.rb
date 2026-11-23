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
          declare i64 @nat_llvm_block(ptr, i64, ptr, i32, i64, i32)
          declare void @nat_llvm_define_method(ptr, i64, ptr, ptr, i32, ptr, i64, i64)
          declare i64 @nat_llvm_arg(ptr, i64)
          declare void @nat_llvm_check_args(ptr, ptr, i64, i64, i32, i64, ptr)
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
          declare i64 @nat_llvm_lambda(ptr, i64, i64)
          declare i64 @nat_llvm_push_block(ptr, ptr, i32)
          declare i64 @nat_llvm_send_dynamic(ptr, i64, i64, ptr, i64, ptr, i32, i32, ptr, i64, i64, i32)
          declare void @nat_llvm_array_concat(ptr, i64, i64)
          declare void @nat_llvm_array_push(i64, i64)
          declare i64 @nat_llvm_splat(ptr, i64)
          declare i64 @nat_llvm_to_array(ptr, i64)
          declare i64 @nat_llvm_yield_array(ptr, i64, i32)
          declare i64 @nat_llvm_regexp(ptr, ptr, i64, i32, i32)
          declare i64 @nat_llvm_string_regexp(ptr, i64, i32, i32)
          declare void @nat_llvm_string_append(ptr, i64, i64)
          declare i64 @nat_llvm_case_equal(ptr, i64, i64, i32)
          declare i64 @nat_llvm_try(ptr, i64, ptr, ptr, ptr, ptr, ptr, i32, i32)
          declare void @nat_llvm_jump(ptr, i64, i32, i64) noreturn
          declare i64 @nat_llvm_match_break(ptr, ptr, i64)
          declare i64 @nat_llvm_match_exception(ptr, ptr, i64)
          declare i64 @nat_llvm_rescued()
          declare i64 @nat_llvm_singleton_class(ptr, i64)
          declare i64 @nat_llvm_with_singleton(ptr, i64, ptr)
          declare void @nat_llvm_alias(ptr, i64, i64, i64)
          declare i64 @nat_llvm_super(ptr, i64, i64, ptr, i64, ptr, i32, i32)
          declare void @nat_llvm_check_keywords(ptr, ptr, i64, ptr, i64, ptr, i32)
          declare i64 @nat_llvm_keyword_get(ptr, ptr, ptr)
          declare i64 @nat_llvm_keyword_present(ptr, ptr, ptr)
          declare i64 @nat_llvm_keyword_rest(ptr, ptr, i64, ptr)
          declare i64 @nat_llvm_pop_keywords(ptr)
          declare void @nat_llvm_hash_merge(ptr, i64, i64)
          declare void @nat_llvm_set_zero(ptr, ptr)
          declare void @nat_llvm_warn(ptr, i64, ptr, i32)
        LLVM

        def initialize(
          instructions,
          source_path:,
          shared: nil,
          function_name: 'nat_llvm_eval',
          scope: nil,
          required_files: {},
          native_extensions: nil,
          region_stack: nil,
          return_point: nil
        )
          @instructions = instructions
          @source_path = source_path
          @stack = []
          @shared = shared || { globals: [], functions: [], serial: 0 }
          @globals = @shared[:globals]
          @function_name = function_name
          @scope = scope
          @required_files = required_files
          @native_extensions = native_extensions
          @allocas = []
          @body = []
          @scope_env = @scope || top_scope
          @region_stack = region_stack
          @return_point = return_point
        end

        def generate
          if @region_stack
            @stack =
              @region_stack.each_with_index.map do |operand, index|
                next operand if operand.is_a?(Integer)
                ptr = value("getelementptr i64, ptr %frame, i64 #{index}")
                value("load volatile i64, ptr #{ptr}")
              end
          end
          if @function_name == 'nat_llvm_eval'
            emit "call void @nat_llvm_set_zero(ptr %env, ptr #{string(@source_path)})"
          end

          transform(@instructions)
          anonymous = @shared.fetch(:anonymous_vars, {}).fetch(@scope_env.object_id, {})
          unless @region_stack
            @body.unshift(
              "  call void @nat_llvm_build_vars(ptr %env, i64 #{@scope_env.fetch(:vars).size + anonymous.size})",
            )
          end
          if @function_name == 'nat_llvm_eval' && @native_extensions&.active?
            @body.unshift('  call void @nat_llvm_init_extensions(ptr %env, i64 %self)')
          end
          emit "ret i64 #{@stack.last || 4}"
          function = <<~LLVM
            define i64 @#{@function_name}(ptr %env, i64 %self#{@function_name == 'nat_llvm_eval' ? '' : ', ptr %args, ptr %block'}#{@region_stack ? ', ptr %frame, ptr %exception' : ''}) uwtable {
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
            #{@native_extensions&.declarations}
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

        def function(body, scope:, return_point: nil)
          fn = name('fn')
          @shared[:functions] << self
            .class
            .new(
              body,
              source_path: @source_path,
              shared: @shared,
              function_name: fn,
              scope: scope,
              return_point: return_point,
              required_files: @required_files,
              native_extensions: @native_extensions,
            )
            .generate
          "@#{fn}"
        end

        def function_args
          @function_name == 'nat_llvm_eval' ? 'null' : '%args'
        end

        def break_point(key)
          @shared[:break_points] ||= {}
          @shared[:break_points][[@source_path, key]] ||= 1_000_000 + @shared[:break_points].size
        end

        def region(body, baseline)
          fn = name('region')
          child =
            self.class.new(
              body,
              source_path: @source_path,
              shared: @shared,
              function_name: fn,
              scope: @scope_env,
              required_files: @required_files,
              native_extensions: @native_extensions,
              region_stack: baseline,
              return_point: @return_point,
            )
          @shared[:functions] << child.generate
          ["@#{fn}", [child.stack.size - 1, 0].max]
        end

        def return_value(value)
          if @region_stack
            raise CompileError, 'LLVM: return from top-level exception region is unsupported' unless @return_point
            emit "call void @nat_llvm_jump(ptr %env, i64 #{value}, i32 1, i64 #{@return_point})"
            emit 'unreachable'
          else
            emit "ret i64 #{value}"
          end
          emit "#{name('unreachable')}:"
          push('4')
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
              native_extensions: @native_extensions,
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

        def anonymous_variable(instruction, name, create: false)
          @shared[:anonymous_vars] ||= {}
          env = instruction.env
          env = env[:outer] while env[:hoist]
          depth = 0
          loop do
            vars = @shared[:anonymous_vars][env.object_id] ||= {}
            vars[name] ||= env.fetch(:vars).size + vars.size if create
            return depth, vars.fetch(name) if vars.key?(name)
            break unless env[:type] == :define_block
            env = env.fetch(:outer)
            env = env[:outer] while env[:hoist]
            depth += 1
          end
          raise CompileError, "LLVM: unbound anonymous #{name}"
        end

        attr_reader :stack, :source_path, :native_extensions

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

        def keyword_names(names)
          values =
            names.map do |name|
              push_call("call i64 @nat_llvm_symbol(ptr #{string(name.to_s)}, i64 #{name.to_s.bytesize})")
              pop
            end
          argument_array(values)
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
