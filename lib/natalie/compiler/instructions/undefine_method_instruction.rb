require_relative './base_instruction'

module Natalie
  class Compiler
    class UndefineMethodInstruction < BaseInstruction
      def to_s
        'undefine_method'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_undef_method(ptr %env, i64 %self, i64 #{transform.pop})")
      end
    end
  end
end
