require_relative './base_instruction'

module Natalie
  class Compiler2
    class EndInstruction < BaseInstruction
      def initialize(label)
        @label = label
      end

      attr_reader :label

      def to_s
        "end #{@label}"
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
