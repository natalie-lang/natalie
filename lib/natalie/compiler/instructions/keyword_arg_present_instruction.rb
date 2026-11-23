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

      def generate(transform)
        transform.exec_and_push(:has_key, "bool_object(args.keyword_arg_present(env, #{transform.intern(@name)}))")
      end
    end
  end
end
