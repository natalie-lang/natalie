require_relative './base_instruction'

module Natalie
  class Compiler
    class WithSingletonInstruction < BaseInstruction
      def has_body?
        true
      end

      def to_s
        'with_singleton'
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_singleton)
        fn = transform.function(body, scope: env)
        transform.push_call("call i64 @nat_llvm_with_singleton(ptr %env, i64 #{transform.pop}, ptr #{fn})")
      end
    end
  end
end
