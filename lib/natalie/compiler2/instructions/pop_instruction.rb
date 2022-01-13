require_relative './base_instruction'

module Natalie
  class Compiler2
    class PopInstruction < BaseInstruction
      def to_s
        'pop'
      end

      # Not sure if we need this to do anything in C++
      # since the "result" on the "stack" is C++ code
      # --not a value we should ignore.
      def to_cpp(transform)
        nil
      end

      def execute(vm)
        vm.pop
      end
    end
  end
end
