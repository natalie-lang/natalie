require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousKeywordSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_keyword_splat_get'
      end

      def generate_llvm(transform)
        depth, index = transform.anonymous_variable(self, :keyword_splat)
        transform.push_call("call i64 @nat_llvm_var_get(ptr %env, i64 #{depth}, i64 #{index})")
      end
    end
  end
end
