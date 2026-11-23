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

      def generate(transform)
        value = transform.pop
        transform.exec("return #{value}")
        transform.push_nil
      end
    end
  end
end
