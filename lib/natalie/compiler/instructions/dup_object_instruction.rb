require_relative './base_instruction'

module Natalie
  class Compiler
    class DupObjectInstruction < BaseInstruction
      def to_s
        'dup_object'
      end

      def generate(transform)
        transform.exec_and_push(:duplicated_value, "#{transform.peek}->duplicate(env)")
      end

      def execute(vm)
        vm.push(vm.peek.dup)
      end
    end
  end
end
