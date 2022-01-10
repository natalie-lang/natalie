module Natalie
  class Compiler2
    class InstructionManager
      def initialize(instructions)
        @instructions = instructions
        @ip = 0
      end

      attr_reader :ip

      def each
        while @ip < @instructions.size
          instruction = @instructions[@ip]
          @ip += 1
          yield instruction
        end
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

        unless expected_label.nil? || instruction.label == expected_label
          raise "unexpected instruction (expected: #{expected_label}, actual: #{instruction.label})"
        end

        instructions
      end
    end
  end
end
