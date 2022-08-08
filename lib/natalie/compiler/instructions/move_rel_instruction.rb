require_relative './base_instruction'

module Natalie
  class Compiler
    class MoveRelInstruction < BaseInstruction
      def initialize(index)
        @index = index
      end

      def to_s
        "move_rel #{@index}"
      end

      def generate(transform)
        raise 'ran out of stack' unless transform.stack.length > @index
        value = transform.stack.slice!(transform.stack.length - 1 - @index)
        transform.push(value)
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
