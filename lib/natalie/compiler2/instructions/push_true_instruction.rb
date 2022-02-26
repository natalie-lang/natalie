require_relative './base_instruction'

module Natalie
  class Compiler2
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
    end
  end
end
