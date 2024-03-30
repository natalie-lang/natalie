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

      def serialize(_)
        matching_label_string = @matching_label.to_s
        [
          instruction_number,
          matching_label_string.bytesize,
          matching_label_string,
        ].pack("Cwa#{matching_label_string.bytesize}")
      end

      def self.deserialize(io)
        size = io.read_ber_integer
        matching_label = io.read(size)
        new(matching_label)
      end
    end
  end
end
