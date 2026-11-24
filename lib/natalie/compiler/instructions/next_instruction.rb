require_relative './base_instruction'

module Natalie
  class Compiler
    class NextInstruction < BaseInstruction
      attr_accessor :jump_point

      def to_s
        'next'
      end

      def generate_llvm(transform)
        transform.return_value(transform.pop)
      end
    end
  end
end
