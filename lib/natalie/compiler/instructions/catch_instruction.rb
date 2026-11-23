require_relative './base_instruction'

module Natalie
  class Compiler
    class CatchInstruction < BaseInstruction
      def to_s
        'catch'
      end

      def generate(_)
        :noop
      end
    end
  end
end
