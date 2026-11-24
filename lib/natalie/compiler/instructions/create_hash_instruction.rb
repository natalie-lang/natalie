require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateHashInstruction < BaseInstruction
      def initialize(count:)
        @count = count
      end

      attr_reader :count

      def to_s
        "create_hash count: #{@count}"
      end

      def generate_llvm(transform)
        items = transform.pop(@count * 2)
        transform.push_call(
          "call i64 @nat_llvm_hash(ptr %env, i64 #{items.size}, ptr #{transform.argument_array(items)})",
        )
      end
    end
  end
end
