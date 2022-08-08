require_relative './base_instruction'

module Natalie
  class Compiler
    class ElseInstruction < BaseInstruction
      def initialize(matching_label)
        @matching_label = matching_label.to_sym
      end

      attr_reader :matching_label

      def to_s
        "else #{@matching_label}"
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
