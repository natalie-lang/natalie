require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayPopInstruction < BaseInstruction
      def to_s
        'array_pop'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_array_pop(i64 #{transform.peek})")
      end
    end
  end
end
