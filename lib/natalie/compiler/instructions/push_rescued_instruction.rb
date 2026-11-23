require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRescuedInstruction < BaseInstruction
      def to_s
        'push_rescued'
      end

      def generate_llvm(transform)
        transform.push_call('call i64 @nat_llvm_rescued()')
      end

      def generate(transform)
        transform.push('bool_object(GlobalEnv::the()->rescued())')
      end
    end
  end
end
