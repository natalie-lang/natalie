require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousKeywordSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_keyword_splat_get'
      end

      def generate(transform)
        transform.push('anon_keyword_splat')
      end
    end
  end
end
