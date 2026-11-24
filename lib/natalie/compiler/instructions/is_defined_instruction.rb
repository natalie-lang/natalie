require_relative './base_instruction'

module Natalie
  class Compiler
    class IsDefinedInstruction < BaseInstruction
      def initialize(type:)
        @type = type
      end

      def to_s
        "is_defined #{@type}"
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :is_defined)
        fn, = transform.region(body, transform.stack.dup)
        frame = transform.argument_array(transform.stack)
        transform.push_call(
          "call i64 @nat_llvm_is_defined(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{fn}, ptr #{frame}, ptr #{transform.string(@type.to_s)})",
        )
      end
    end
  end
end
