require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRescuedInstruction < BaseInstruction
      def to_s
        'push_rescued'
      end

      def generate(transform)
        transform.push('bool_object(GlobalEnv::the()->rescued())')
      end

      def execute(vm)
        vm.push(vm.rescued)
      end
    end
  end
end
