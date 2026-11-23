require_relative './base_instruction'

module Natalie
  class Compiler
    class NextInstruction < BaseInstruction
      def to_s
        'next'
      end

      def generate_llvm(transform)
        transform.return_value(transform.pop)
      end

      def generate(transform)
        transform.exec("return #{transform.pop}")
        transform.push_nil
      end
    end
  end
end
