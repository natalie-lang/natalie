require_relative './base_instruction'

module Natalie
  class Compiler
    class DupInstruction < BaseInstruction
      def to_s
        'dup'
      end

      def generate_llvm(transform)
        transform.push(transform.peek)
      end
    end
  end
end
