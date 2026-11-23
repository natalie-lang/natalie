require_relative './base_instruction'

module Natalie
  class Compiler
    class PushLastMatchInstruction < BaseInstruction
      def to_s
        'last_match'
      end

      def generate_llvm(transform)
        transform.push_call('call i64 @nat_llvm_last_match(ptr %env)')
      end

      def generate(transform)
        transform.push('Value(env->last_match())')
      end
    end
  end
end
