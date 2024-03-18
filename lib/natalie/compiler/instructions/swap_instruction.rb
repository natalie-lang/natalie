require_relative './base_instruction'

module Natalie
  class Compiler
    class SwapInstruction < BaseInstruction
      def to_s
        'swap'
      end

      def generate(transform)
        top = transform.pop
        one = transform.pop
        transform.push(top)
        transform.push(one)
      end

      def execute(vm)
        top = vm.pop
        one = vm.pop
        vm.push(top)
        vm.push(one)
      end

      def serialize
        [instruction_number].pack('C')
      end

      def self.deserialize(_)
        new
      end
    end
  end
end
