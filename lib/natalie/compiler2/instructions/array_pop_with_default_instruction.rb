require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayPopWithDefaultInstruction < BaseInstruction
      def to_s
        'array_pop_with_default'
      end

      def generate(transform)
        default = transform.pop
        ary = transform.memoize(:ary, transform.peek)
        code = "(#{ary}->as_array()->is_empty() ? #{default} : #{ary}->as_array()->pop())"
        transform.exec_and_push(:last_item_of_array, code)
      end

      def execute(vm)
        default = vm.pop
        ary = vm.peek
        vm.push(ary.empty? ? default : ary.pop)
      end
    end
  end
end
