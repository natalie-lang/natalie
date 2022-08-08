require_relative './base_instruction'

module Natalie
  class Compiler
    class DupInstruction < BaseInstruction
      def to_s
        'dup'
      end

      def generate(transform)
        transform.push(transform.peek)
      end

      def execute(vm)
        vm.push(vm.peek)
      end
    end
  end
end
