module Natalie
  class Compiler2
    class CppBackend
      class Transform
        def initialize(instructions, top:, compiler_context:, env: nil, parent_env: nil)
          @instructions = InstructionManager.new(instructions)
          @result_stack = []
          @top = top
          @code = []
          @compiler_context = compiler_context
          if env
            @env = env
          else
            @env = { vars: {}, parent: parent_env }
          end
          @stack = []
        end

        def ip
          @instructions.ip
        end

        attr_reader :stack

        def transform(result_prefix = nil)
          @instructions.walk do |instruction|
            instruction.generate(self)
          end
          consume(@code + @stack, result_prefix)
        end

        def semicolon(line)
          line =~ /[\{\};]$/ ? line : "#{line};"
        end

        def exec(code)
          @code << consume(code)
        end

        def push(result)
          @stack << result
        end

        def pop
          raise 'ran out of stack' unless @stack.any?

          @stack.pop
        end

        def push_scope
          @env = { parent: @env, vars: {} }
        end

        def pop_scope
          @env = @env[:parent]
        end

        def vars
          @env[:vars]
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
          t = Transform.new(instructions, top: @top, compiler_context: @compiler_context, parent_env: @env)
          yield(t)
        end

        def with_same_scope(instructions)
          t = Transform.new(instructions, top: @top, compiler_context: @compiler_context, env: @env)
          yield(t)
        end

        def consume(lines, result_prefix = nil)
          lines = Array(lines)
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
