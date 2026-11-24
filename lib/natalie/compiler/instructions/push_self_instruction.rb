require_relative './base_instruction'

module Natalie
  class Compiler
    class PushSelfInstruction < BaseInstruction
      def to_s
        'push_self'
      end

      def generate_llvm(transform)
        transform.push('%self')
      end
    end
  end
end
