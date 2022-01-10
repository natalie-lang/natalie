require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushStringInstruction < BaseInstruction
      def initialize(string)
        @string = string
      end

      def to_s
        "push_string #{@string.inspect}"
      end
    end
  end
end
