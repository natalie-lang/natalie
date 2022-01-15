require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayPopInstruction < BaseInstruction
      def to_s
        'array_pop'
      end

      def generate(transform)
        ary = transform.peek
        transform.push("#{ary}->as_array()->pop()")
      end

      def execute(vm)
        ary = vm.peek
        vm.push(ary.pop)
      end
    end
  end
end
