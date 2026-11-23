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
          declare i64 @nat_llvm_object_class()
          declare i64 @nat_llvm_const(ptr, i64, i64, ptr, i32, i32)
          declare i64 @nat_llvm_send(ptr, i64, i64, ptr, i64, ptr, i32, i32, ptr, i64)
          declare void @nat_llvm_set_zero(ptr, ptr)
          declare void @nat_llvm_warn(ptr, i64, ptr, i32)
        LLVM

        def initialize(instructions, source_path:)
          @instructions = instructions
          @source_path = source_path
          @stack = []
          @globals = []
          @allocas = []
          @body = []
          @locals = {}
          @serial = 0
        end

        def generate
          emit "call void @nat_llvm_set_zero(ptr %env, ptr #{string(@source_path)})"
          transform(@instructions)
          emit "ret i64 #{@stack.last || 4}"
          <<~LLVM
            ; Natalie experimental LLVM backend (64-bit hosts)
            #{@globals.join("\n")}
            #{DECLARATIONS}
            define i64 @nat_llvm_eval(ptr %env, i64 %self) uwtable {
            entry:
            #{@allocas.join("\n")}
            #{@body.join("\n")}
            }
          LLVM
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

        def local(instruction)
          @locals[instruction.meta.object_id] ||= slot('local')
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
          @serial += 1
          "#{prefix}#{@serial}"
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
