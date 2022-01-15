require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushSelfInstruction < BaseInstruction
      def to_s
        'push_self'
      end

      def generate(transform)
        transform.push('self')
      end

      def execute(vm)
        vm.push(vm.self)
      end
    end
  end
end
