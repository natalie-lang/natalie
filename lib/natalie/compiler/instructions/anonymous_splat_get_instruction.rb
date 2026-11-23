require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_get'
      end

      def generate(transform)
        transform.push('anon_splat')
      end
    end
  end
end
