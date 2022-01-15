require_relative './base_instruction'

module Natalie
  class Compiler2
    class ElseInstruction < BaseInstruction
      def to_s
        'else'
      end

      def generate(_)
        :noop
      end

      def execute(_)
        :halt
      end
    end
  end
end
