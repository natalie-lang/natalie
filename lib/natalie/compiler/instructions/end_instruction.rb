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

      def serialize
        label_string = matching_label.to_s
        [
          instruction_number,
          label_string.bytesize,
          label_string,
        ].pack("Cwa*")
      end

      def self.deserialize(io)
        size = io.read_ber_integer
        matching_label = io.read(size).to_sym
        new(matching_label)
      end
    end
  end
end
