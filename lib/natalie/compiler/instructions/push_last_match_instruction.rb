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
        vm.push($~)
      end
    end
  end
end
