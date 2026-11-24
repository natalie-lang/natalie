require_relative './base_instruction'

module Natalie
  class Compiler
    class ToArrayInstruction < BaseInstruction
      def to_s
        'to_array'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_to_array(ptr %env, i64 #{transform.pop})")
      end
    end
  end
end
