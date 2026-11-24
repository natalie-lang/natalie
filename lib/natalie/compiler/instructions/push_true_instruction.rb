require_relative './base_instruction'

module Natalie
  class Compiler
    class PushTrueInstruction < BaseInstruction
      def to_s
        'push_true'
      end

      def generate_llvm(transform)
        transform.push('20')
      end
    end
  end
end
