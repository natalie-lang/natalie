require_relative './base_instruction'

module Natalie
  class Compiler
    class PushMainInstruction < BaseInstruction
      def to_s
        'push_main'
      end

      def generate(transform)
        transform.push('GlobalEnv::the()->main_obj()')
      end

      def execute(vm)
        vm.push(vm.main)
      end
    end
  end
end
