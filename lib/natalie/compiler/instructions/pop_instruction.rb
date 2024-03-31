require_relative './base_instruction'

module Natalie
  class Compiler
    class PopInstruction < BaseInstruction
      def to_s
        'pop'
      end

      def generate(transform)
        transform.pop
      end

      def execute(vm)
        vm.pop
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
