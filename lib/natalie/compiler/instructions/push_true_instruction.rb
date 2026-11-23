require_relative './base_instruction'

module Natalie
  class Compiler
    class PushTrueInstruction < BaseInstruction
      def to_s
        'push_true'
      end

      def generate(transform)
        transform.push('Value::True()')
      end
    end
  end
end
