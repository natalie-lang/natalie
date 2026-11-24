require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayConcatInstruction < BaseInstruction
      def to_s
        'array_concat'
      end

      def generate_llvm(transform)
        other = transform.pop
        transform.emit "call void @nat_llvm_array_concat(ptr %env, i64 #{transform.peek}, i64 #{other})"
      end
    end
  end
end
