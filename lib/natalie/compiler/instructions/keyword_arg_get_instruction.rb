require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "keyword_arg_get #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(@name, "args.keyword_arg(env, #{transform.intern(@name)})")
      end
    end
  end
end
