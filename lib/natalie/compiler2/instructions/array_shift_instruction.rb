require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayShiftInstruction < BaseInstruction
      def to_s
        'array_shift'
      end

      def generate(transform)
        ary = transform.peek
        transform.push("#{ary}->as_array()->shift()")
      end

      def execute(vm)
        ary = vm.peek
        vm.push(ary.shift)
      end
    end
  end
end
