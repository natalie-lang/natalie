require_relative './base_instruction'

module Natalie
  class Compiler
    class PushTrueInstruction < BaseInstruction
      def to_s
        'push_true'
      end

      def generate(transform)
        transform.push('Value(TrueObject::the())')
      end

      def execute(vm)
        vm.push(true)
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
