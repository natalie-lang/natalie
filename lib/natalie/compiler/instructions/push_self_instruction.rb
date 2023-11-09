require_relative './base_instruction'

module Natalie
  class Compiler
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

      def serialize
        [instruction_number].pack('C')
      end
    end
  end
end
