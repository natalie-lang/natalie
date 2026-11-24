require_relative './base_instruction'

module Natalie
  class Compiler
    class PushNilInstruction < BaseInstruction
      def to_s
        'push_nil'
      end

      def generate_llvm(transform)
        transform.push('4')
      end
    end
  end
end
