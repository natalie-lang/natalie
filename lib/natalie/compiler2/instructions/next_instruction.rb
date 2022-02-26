require_relative './base_instruction'

module Natalie
  class Compiler2
    class NextInstruction < BaseInstruction
      def to_s
        'next'
      end

      def generate(transform)
        transform.exec("return #{transform.pop}")
        transform.push('Value(NilObject::the())')
      end

      def execute(vm)
        # value is on stack already
        :next
      end
    end
  end
end
