require_relative './base_instruction'

module Natalie
  class Compiler2
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate(transform)
        transform.exec("return #{transform.pop}")
        transform.push('Value(NilObject::the())')
      end

      def execute(vm)
        :return
      end
    end
  end
end
