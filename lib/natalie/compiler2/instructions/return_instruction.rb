require_relative './base_instruction'

module Natalie
  class Compiler2
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate(transform)
        transform.exec("return #{transform.pop}")
      end

      def execute(vm)
        :halt
      end
    end
  end
end
