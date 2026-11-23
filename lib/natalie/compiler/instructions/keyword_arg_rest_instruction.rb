require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgRestInstruction < BaseInstruction
      def initialize(without:)
        @without = without
      end

      def to_s
        "keyword_arg_rest without: #{@without.join(', ')}"
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_keyword_rest(ptr %env, ptr %args, i64 #{@without.size}, ptr #{transform.keyword_names(@without)})",
        )
      end

      def generate(transform)
        transform.exec_and_push(
          'kwarg_rest',
          "args.keyword_arg_rest(env, { #{@without.map { |n| transform.intern(n) }.join(', ')} })",
        )
      end
    end
  end
end
