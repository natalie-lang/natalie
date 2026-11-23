require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousKeywordSplatSetInstruction < BaseInstruction
      def to_s
        'anonymous_keyword_splat_set'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("Value anon_keyword_splat = #{value}")
      end
    end
  end
end
