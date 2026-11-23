require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_get'
      end

      def generate_llvm(transform)
        depth, index = transform.anonymous_variable(self, :splat)
        transform.push_call("call i64 @nat_llvm_var_get(ptr %env, i64 #{depth}, i64 #{index})")
      end

      def generate(transform)
        transform.push('anon_splat')
      end
    end
  end
end
