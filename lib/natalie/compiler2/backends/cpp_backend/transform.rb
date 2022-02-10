module Natalie
  class Compiler2
    class CppBackend
      class Transform
        def initialize(instructions, stack: [], top:, compiler_context:, env: nil, outer_env: nil, block: true)
          @instructions = InstructionManager.new(instructions)
          @result_stack = []
          @top = top
          @code = []
          @compiler_context = compiler_context
          if env
            @env = env
          else
            @env = { vars: {}, outer: outer_env, block: block }
          end
          @stack = stack
        end

        def ip
          @instructions.ip
        end

        attr_reader :stack

        def transform(result_prefix = nil)
          @instructions.walk do |instruction|
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

        def push_scope
          @env = { outer: @env, vars: {} }
        end

        def pop_scope
          @env = @env[:outer]
        end

        def vars
          @env[:vars]
        end

        def find_var(name, local_only: false)
          env = @env
          depth = 0
          loop do
            if env[:vars].key?(name)
              return [depth, env.dig(:vars, name)]
            end
            if env[:block] && env[:outer] && !local_only
              env = env[:outer]
              depth += 1
            else
              break
            end
          end
        end

        def fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: nil)
          @instructions.fetch_block(until_instruction: until_instruction, expected_label: expected_label)
        end

        def temp(name)
          name = name.to_s.gsub(/[^a-zA-Z_]/, '')
          n = @compiler_context[:var_num] += 1
          "#{@compiler_context[:var_prefix]}#{name}#{n}"
        end

        def with_new_scope(instructions, block: false)
          t = Transform.new(instructions, top: @top, compiler_context: @compiler_context, outer_env: @env, block: block)
          yield(t)
        end

        def with_same_scope(instructions)
          t = Transform.new(instructions, stack: @stack.dup, top: @top, compiler_context: @compiler_context, env: @env)
          yield(t)
        end

        def consume(lines, result_prefix = nil)
          lines = Array(lines).compact
          out = []
          while lines.any?
            line = lines.shift
            next if line.nil?
            lines.empty? ? out << "#{result_prefix} #{semicolon(line)}" : out << semicolon(line)
          end
          out.join("\n")
        end

        def top(code)
          @top << Array(code).join("\n")
        end
      end
    end
  end
end
