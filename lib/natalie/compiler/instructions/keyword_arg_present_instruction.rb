require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgPresentInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "keyword_arg_present #{@name}"
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_keyword_present(ptr %env, ptr %args, ptr #{transform.string(@name.to_s)})",
        )
      end
    end
  end
end
