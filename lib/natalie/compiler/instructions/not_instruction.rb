require_relative './base_instruction'

module Natalie
  class Compiler
    class NotInstruction < BaseInstruction
      def to_s
        'not'
      end

      def generate(transform)
        obj = transform.pop
        transform.push("#{obj}.public_send(env, \"!\"_s)")
      end
    end
  end
end
