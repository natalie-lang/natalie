require_relative './base_instruction'

module Natalie
  class Compiler
    class HashMergeInstruction < BaseInstruction
      def to_s
        'hash_merge'
      end

      def generate_llvm(transform)
        other = transform.pop
        transform.emit "call void @nat_llvm_hash_merge(ptr %env, i64 #{transform.peek}, i64 #{other})"
      end
    end
  end
end
