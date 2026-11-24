require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatSetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_set'
      end

      def generate_llvm(transform)
        depth, index = transform.anonymous_variable(self, :splat, create: true)
        transform.emit "call void @nat_llvm_var_set(ptr %env, i64 #{depth}, i64 #{index}, i64 #{transform.pop})"
      end
    end
  end
end
