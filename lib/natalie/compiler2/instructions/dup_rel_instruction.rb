require_relative './base_instruction'

module Natalie
  class Compiler2
    class DupRelInstruction < BaseInstruction
      # offset = 0 is equivalent to DupInstruction
      def initialize(offset)
        @offset = offset
      end

      def to_s
        "dup_rel #{@offset}"
      end

      def generate(transform)
        raise 'ran out of stack' unless transform.stack.length > @offset
        transform.push(transform.stack[transform.stack.length - 1 - @offset])
      end

      def execute(vm)
        raise 'ran out of stack' unless vm.stack.length > @offset
        vm.push(vm.stack[vm.stack.length - 1 - @offset])
      end
    end
  end
end
