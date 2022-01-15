require_relative './base_instruction'

module Natalie
  class Compiler2
    class PopInstruction < BaseInstruction
      def to_s
        'pop'
      end

      def generate(transform)
        transform.pop
      end

      def execute(vm)
        vm.pop
      end
    end
  end
end
