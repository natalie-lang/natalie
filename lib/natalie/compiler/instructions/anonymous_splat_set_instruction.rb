require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatSetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_set'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("Value anon_splat = #{value}")
      end
    end
  end
end
