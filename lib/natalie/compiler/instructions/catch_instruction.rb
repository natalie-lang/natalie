require_relative './base_instruction'

module Natalie
  class Compiler
    class CatchInstruction < BaseInstruction
      def to_s
        'catch'
      end

      def generate_llvm(_)
        # The enclosing try instruction extracts the handler body.
      end
    end
  end
end
