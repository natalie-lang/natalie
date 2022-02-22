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

      def insert_after(instructions)
        insert_at(@ip, instructions)
      end

      def insert_at(ip, instructions)
        @instructions.insert(ip, *Array(instructions))
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

      def find_next(instruction_klass)
        ip = @ip
        while ip < @instructions.size
          instruction = @instructions[ip]
          if instruction.is_a?(instruction_klass)
            return ip, instruction
          end
          ip += 1
        end
        raise 'ran out of instructions'
      end
    end
  end
end
