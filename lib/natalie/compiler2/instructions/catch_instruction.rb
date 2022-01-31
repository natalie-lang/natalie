require_relative './base_instruction'

module Natalie
  class Compiler2
    class CatchInstruction < BaseInstruction
      def to_s
        'catch'
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
