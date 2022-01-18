require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayPopInstruction < BaseInstruction
      def initialize(with_default: false)
        @with_default = with_default
      end

      def to_s
        s = 'array_pop'
        s << ' with_default' if @with_default
        s
      end

      def generate(transform)
        default = transform.pop if @with_default
        ary = transform.memoize(:ary, transform.peek)
        code = "#{ary}->as_array()->pop()"
        if @with_default
          code = "(#{ary}->is_empty() ? #{default} : #{code})"
        end
        transform.exec_and_push(:first_item_of_array, code)
      end

      def execute(vm)
        default = vm.pop if @with_default
        ary = vm.peek
        vm.push(ary.empty? ? default : ary.pop)
      end
    end
  end
end
