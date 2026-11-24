require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayShiftInstruction < BaseInstruction
      def to_s
        'array_shift'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_array_shift(i64 #{transform.peek})")
      end
    end
  end
end
