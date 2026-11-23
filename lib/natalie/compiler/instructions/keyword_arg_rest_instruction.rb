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

      def generate(transform)
        transform.exec_and_push(
          'kwarg_rest',
          "args.keyword_arg_rest(env, { #{@without.map { |n| transform.intern(n) }.join(', ')} })",
        )
      end
    end
  end
end
