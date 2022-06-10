require_relative './base_instruction'

module Natalie
  class Compiler2
    # NOTE: I'm not convinced this is the best name for this instruction,
    # as it does not *always* wrap the object in an array -- only if the
    # object is not already an array or array-like.
    class ArrayWrapInstruction < BaseInstruction
      def to_s
        'array_wrap'
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:array, "splat(env, #{obj})")
      end

      def execute(vm)
        obj = vm.pop
        vm.push(Array(obj))
      end
    end
  end
end
