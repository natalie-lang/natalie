require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousBlockGetInstruction < BaseInstruction
      def to_s
        'anonymous_block_get'
      end

      def generate(transform)
        transform.push('anon_block')
      end
    end
  end
end
