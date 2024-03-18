require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayShiftWithDefaultInstruction < BaseInstruction
      def to_s
        'array_shift_with_default'
      end

      def generate(transform)
        default = transform.pop
        ary = transform.memoize(:ary, transform.peek)
        code = "(#{ary}->as_array()->is_empty() ? #{default} : #{ary}->as_array()->shift())"
        transform.exec_and_push(:first_item_of_array, code)
      end

      def execute(vm)
        default = vm.pop
        ary = vm.peek
        vm.push(ary.empty? ? default : ary.shift)
      end

      def serialize
        [instruction_number].pack('C')
      end

      def self.deserialize(_)
        new
      end
    end
  end
end
