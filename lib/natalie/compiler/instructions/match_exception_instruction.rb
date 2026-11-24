require_relative './base_instruction'

module Natalie
  class Compiler
    class MatchExceptionInstruction < BaseInstruction
      def to_s
        'match_exception'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_match_exception(ptr %env, ptr %exception, i64 #{transform.pop})")
      end
    end
  end
end
