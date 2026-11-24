require_relative './base_instruction'

module Natalie
  class Compiler
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate_llvm(transform)
        transform.return_value(transform.pop)
      end
    end
  end
end
