require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayConcatInstruction < BaseInstruction
      def to_s
        'array_concat'
      end

      def generate(transform)
        ary2 = transform.pop
        ary = transform.peek
        # NOTE: ArrayObject::push_splat() is better than concat() because it
        # can handle when the given value is not an array.
        transform.exec("#{ary}->as_array()->push_splat(env, #{ary2})")
      end

      def execute(vm)
        ary2 = vm.pop
        ary = vm.peek
        ary.concat(Array(ary2))
      end
    end
  end
end
