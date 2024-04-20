require_relative './base_instruction'

module Natalie
  class Compiler
    class PushLastMatchInstruction < BaseInstruction
      def to_s
        'last_match'
      end

      def generate(transform)
        transform.push("Value(env->last_match())")
      end

      def execute(vm)
        vm.push(vm.global_variables[:$~])
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
