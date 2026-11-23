require_relative './base_instruction'

module Natalie
  class Compiler
    class ToArrayInstruction < BaseInstruction
      def to_s
        'to_array'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_to_array(ptr %env, i64 #{transform.pop})")
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:array, "to_ary_for_masgn(env, #{obj})")
      end
    end
  end
end
