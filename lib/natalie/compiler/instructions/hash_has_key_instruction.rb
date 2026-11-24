require_relative './base_instruction'

module Natalie
  class Compiler
    class HashHasKeyInstruction < BaseInstruction
      def initialize(key)
        raise "expected symbol but got: #{key.inspect}" unless key.is_a?(Symbol)

        @key = key
      end

      def to_s
        "hash_has_key #{@key.inspect}"
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_hash_has_key(ptr %env, i64 #{transform.peek}, ptr #{transform.string(@key.to_s)})",
        )
      end
    end
  end
end
