require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayPushInstruction < BaseInstruction
      def to_s
        'array_push'
      end

      def generate_llvm(transform)
        value = transform.pop
        transform.emit "call void @nat_llvm_array_push(i64 #{transform.peek}, i64 #{value})"
      end
    end
  end
end
