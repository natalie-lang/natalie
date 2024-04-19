require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayPopInstruction < BaseInstruction
      def to_s
        'array_pop'
      end

      def generate(transform)
        ary = transform.peek
        transform.exec_and_push(:last_item_of_array, "#{ary}->as_array()->pop()")
      end

      def execute(vm)
        ary = vm.peek
        vm.push(ary.pop)
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
