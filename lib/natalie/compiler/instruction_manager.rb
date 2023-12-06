module Natalie
  class Compiler
    class InstructionManager
      def initialize(instructions, env: nil)
        unless instructions.is_a?(Array)
          raise 'expected array'
        end
        @instructions = instructions
        @break_point = 0
        @env = env
        reset
      end

      attr_accessor :ip

      def to_a
        @instructions
      end

      def each(&block)
        @instructions.each(&block)
      end

      def each_with_index(&block)
        @instructions.each_with_index(&block)
      end

      def map(&block)
        @instructions.map(&block)
      end

      def walk
        while @ip < @instructions.size
          instruction = self.next
          yield instruction
        end
        reset
        self
      end

      def reset
        @ip = 0
      end

      def next
        instruction = @instructions[@ip]
        @ip += 1
        instruction
      end

      def peek
        @instructions[@ip]
      end

      # If you've already grabbed an instruction with `next` and you're working on it,
      # you will need to use an offset of 2 to get back to the previous one.
      def peek_back(offset = 1)
        @instructions[@ip - offset]
      end

      def replace_at(ip, new_instructions)
        raise 'expected array of instructions' unless new_instructions.all? { |i| i.is_a?(BaseInstruction) }

        @instructions[ip,1] = new_instructions
        EnvBuilder.new(new_instructions, env: @env).process
      end

      def replace_current(instructions)
        replace_at(@ip - 1, instructions)
      end

      def fetch_block(until_instruction: EndInstruction, expected_label: nil)
        instructions = []

        instruction = nil
        loop do
          raise 'ran out of instructions' if @ip >= @instructions.size

          instruction = @instructions[@ip]
          @ip += 1

          instructions << instruction
          break if instruction.is_a?(until_instruction)

          instructions += fetch_block(expected_label: instruction.class.label) if instruction.has_body?
        end

        unless expected_label.nil? || instruction.matching_label == expected_label
          raise "unexpected instruction (expected: #{expected_label}, actual: #{instruction.matching_label})"
        end

        instructions
      end

      def find_previous(instruction_class)
        ip = @ip
        while ip != 0
          return @instructions[ip] if @instructions[ip].is_a?(instruction_class)
          ip -= 1
        end
      end

      def next_break_point
        @break_point += 1
      end
    end
  end
end
