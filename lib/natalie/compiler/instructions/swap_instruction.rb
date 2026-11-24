require_relative './base_instruction'

module Natalie
  class Compiler
    class SwapInstruction < BaseInstruction
      def to_s
        'swap'
      end

      def generate_llvm(transform)
        top = transform.pop
        one = transform.pop
        transform.push(top)
        transform.push(one)
      end
    end
  end
end
