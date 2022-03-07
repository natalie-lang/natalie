module Natalie
  class Compiler2
    class InstructionManager
      def initialize(instructions)
        if instructions.is_a?(InstructionManager)
          @instructions = instructions.instance_variable_get(:@instructions)
        else
          @instructions = instructions
        end
        @ip = 0
      end

      attr_accessor :ip

      def walk
        while @ip < @instructions.size
          instruction = self.next
          yield instruction
        end
        @instructions
      end

      def next
        instruction = @instructions[@ip]
        @ip += 1
        instruction
      end

      def peek
        @instructions[@ip]
      end

      def insert_left(instructions)
        count = insert_at(@ip - 1, instructions)
        @ip += count
      end

      def insert_right(instructions)
        insert_at(@ip, instructions)
      end

      def insert_at(ip, instructions)
        instructions = Array(instructions)
        @instructions.insert(ip, *instructions)
        instructions.size
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

          instructions += fetch_block(expected_label: instruction.label) if instruction.has_body?
        end

        unless expected_label.nil? || instruction.matching_label == expected_label
          raise "unexpected instruction (expected: #{expected_label}, actual: #{instruction.matching_label})"
        end

        instructions
      end
    end
  end
end
