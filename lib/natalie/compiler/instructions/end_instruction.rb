require_relative './base_instruction'

module Natalie
  class Compiler
    class EndInstruction < BaseInstruction
      def initialize(matching_label)
        @matching_label = matching_label.to_sym
      end

      attr_reader :matching_label

      attr_accessor :matching_instruction

      def to_s
        "end #{@matching_label}"
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
