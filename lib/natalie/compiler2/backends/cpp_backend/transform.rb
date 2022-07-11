module Natalie
  class Compiler2
    class CppBackend
      class Transform
        def initialize(instructions, stack: [], top:, compiler_context:)
          @instructions = InstructionManager.new(instructions)
          @result_stack = []
          @top = top
          @code = []
          @compiler_context = compiler_context
          @stack = stack
        end

        def ip
          @instructions.ip
        end

        attr_reader :stack

        def transform(result_prefix = nil)
          @instructions.walk do |instruction|
            @env = instruction.env
            instruction.generate(self)
          end
          @code << @stack.pop
          consume(@code, result_prefix)
        end

        def semicolon(line)
          line =~ /[\{\};]$/ ? line : "#{line};"
        end

        def exec(code)
          @code << consume(code)
        end

        def memoize(name, code)
          result = temp(name)
          @code << consume(code, "auto #{result} = ")
          result
        end

        def push(result)
          @stack << result
        end

        def exec_and_push(name, code)
          result = memoize(name, code)
          push(result)
          result
        end

        def pop
          raise 'ran out of stack' unless @stack.any?

          @stack.pop
        end

        def peek
          raise 'ran out of stack' unless @stack.any?

          @stack.last
        end

        def vars
          @env[:vars]
        end

        def find_var(name, local_only: false)
          env = @env
          depth = 0

          loop do
            env = env[:outer] while env[:hoist]
            if env.fetch(:vars).key?(name)
              var = env.dig(:vars, name)
              return [depth, var]
            end
            if env[:block] && !local_only && (outer = env.fetch(:outer))
              env = outer
              depth += 1
            else
              break
            end
          end

          raise "unknown variable #{name}"
        end

        def fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: nil)
          @instructions.fetch_block(until_instruction: until_instruction, expected_label: expected_label)
        end

        def temp(name)
          name = name.to_s.gsub(/[^a-zA-Z_]/, '')
          n = @compiler_context[:var_num] += 1
          "#{@compiler_context[:var_prefix]}#{name}#{n}"
        end

        def with_new_scope(instructions)
          t = Transform.new(instructions, top: @top, compiler_context: @compiler_context)
          yield(t)
        end

        def with_same_scope(instructions)
          stack = @stack.dup
          t = Transform.new(instructions, stack: stack, top: @top, compiler_context: @compiler_context)
          yield(t)
          @stack_sizes << stack.size if @stack_sizes
        end

        # truncate resulting stack to minimum size of any
        # code branch within this block
        def normalize_stack
          @stack_sizes = []
          yield
          @stack_sizes << stack.size
          @stack[@stack_sizes.min..] = []
          @stack_sizes = nil
        end

        def consume(lines, result_prefix = nil)
          lines = Array(lines).compact
          out = []
          while lines.any?
            line = lines.shift
            next if line.nil?
            next if result_prefix.nil? && useless_value?(line)
            if lines.empty? && line !~ /^\s*env->raise|^\s*break;?$/
              out << "#{result_prefix} #{semicolon(line)}"
            else
              out << semicolon(line)
            end
          end
          out.join("\n")
        end

        def top(code)
          @top << Array(code).join("\n")
        end

        def useless_value?(value)
          !!(value =~ /^Value\((False|Nil|True)Object::the\(\)\)$/)
        end
      end
    end
  end
end
