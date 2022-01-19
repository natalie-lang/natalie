require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushFalseInstruction < BaseInstruction
      def to_s
        'push_false'
      end

      def generate(transform)
        transform.push('FalseObject::the()')
      end

      def execute(vm)
        vm.push(false)
      end
    end
  end
end
