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

      def serialize
        [instruction_number].pack('C')
      end
    end
  end
end
