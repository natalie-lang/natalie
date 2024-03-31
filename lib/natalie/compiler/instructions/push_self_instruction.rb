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

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
