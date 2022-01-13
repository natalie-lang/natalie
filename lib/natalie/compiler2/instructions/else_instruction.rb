require_relative './base_instruction'

module Natalie
  class Compiler2
    class ElseInstruction < BaseInstruction
      def to_s
        'else'
      end

      def to_cpp(_)
        nil
      end

      def execute(_)
        :halt
      end
    end
  end
end
