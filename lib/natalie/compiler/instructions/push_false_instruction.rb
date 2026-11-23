require_relative './base_instruction'

module Natalie
  class Compiler
    class PushFalseInstruction < BaseInstruction
      def to_s
        'push_false'
      end

      def generate_llvm(transform)
        transform.push('0')
      end

      def generate(transform)
        transform.push('Value::False()')
      end
    end
  end
end
