require_relative './base_instruction'

module Natalie
  class Compiler
    class DupObjectInstruction < BaseInstruction
      def to_s
        'dup_object'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_dup_object(ptr %env, i64 #{transform.peek})")
      end
    end
  end
end
