require_relative './base_instruction'

module Natalie
  class Compiler
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("return #{value}")
        transform.push_nil
      end

      def execute(_vm)
        :return
      end
    end
  end
end
