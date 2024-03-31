require_relative './base_instruction'

module Natalie
  class Compiler
    class WhileBodyInstruction < BaseInstruction
      def to_s
        'while_body'
      end

      def has_body?
        # Technically this instruction does not *have* a body, but rather
        # it *is* part of the body of the WhileInstruction. Trust me,
        # this should be false. :-)
        false
      end

      def generate(transform)
        :noop
      end

      def execute(vm)
        :halt
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
