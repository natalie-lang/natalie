require_relative './base_instruction'

module Natalie
  class Compiler
    class PopInstruction < BaseInstruction
      def to_s
        'pop'
      end

      def generate_llvm(transform)
        transform.pop
      end
    end
  end
end
