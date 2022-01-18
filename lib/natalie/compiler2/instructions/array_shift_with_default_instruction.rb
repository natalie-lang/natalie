require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayShiftWithDefaultInstruction < BaseInstruction
      def to_s
        'array_shift_with_default'
      end

      def generate(transform)
        default = transform.pop
        ary = transform.memoize(:ary, transform.peek)
        code = "(#{ary}->is_empty() ? #{default} : #{ary}->as_array()->shift())"
        transform.exec_and_push(:first_item_of_array, code)
      end

      def execute(vm)
        default = vm.pop
        ary = vm.peek
        vm.push(ary.empty? ? default : ary.shift)
      end
    end
  end
end
