require_relative './base_instruction'

module Natalie
  class Compiler2
    class DupRelInstruction < BaseInstruction
      def initialize(index)
        @index = index
      end

      def to_s
        "dup_rel #{@index}"
      end

      def generate(transform)
        raise 'ran out of stack' unless transform.stack.length > @index
        transform.push(transform.stack[transform.stack.length - 1 - @index])
      end

      def execute(vm)
        raise 'ran out of stack' unless vm.stack.length > @index
        vm.push(vm.stack[vm.stack.length - 1 - @index])
      end
    end
  end
end
