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

      def generate_llvm(transform)
        raise CompileError, 'LLVM: instruction stack underflow' unless transform.stack.size > @index
        transform.push(transform.stack.delete_at(-1 - @index))
      end
    end
  end
end
