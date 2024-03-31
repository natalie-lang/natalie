require_relative './base_instruction'

module Natalie
  class Compiler
    class NextInstruction < BaseInstruction
      def to_s
        'next'
      end

      def generate(transform)
        transform.exec("return #{transform.pop}")
        transform.push_nil
      end

      def execute(vm)
        # value is on stack already
        :next
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
