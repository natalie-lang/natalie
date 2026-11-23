require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousBlockSetInstruction < BaseInstruction
      def to_s
        'anonymous_block_set'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("Value anon_block = #{value}")
      end
    end
  end
end
