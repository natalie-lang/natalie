require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayConcatInstruction < BaseInstruction
      def to_s
        'array_concat'
      end

      def generate(transform)
        ary2 = transform.pop
        ary = transform.peek
        transform.exec("#{ary}->as_array()->concat(env, 1, &#{ary2})")
      end

      def execute(vm)
        ary2 = vm.pop
        ary = vm.peek
        ary.concat(ary2)
      end
    end
  end
end
