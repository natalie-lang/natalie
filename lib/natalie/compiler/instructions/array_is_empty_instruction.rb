require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayIsEmptyInstruction < BaseInstruction
      def to_s
        'array_is_empty'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_array_empty(i64 #{transform.peek})")
      end
    end
  end
end
