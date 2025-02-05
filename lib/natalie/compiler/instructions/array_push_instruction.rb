require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayPushInstruction < BaseInstruction
      def to_s
        'array_push'
      end

      def generate(transform)
        value = transform.pop
        ary = transform.peek
        transform.exec("#{ary}.as_array()->push(#{value})")
      end

      def execute(vm)
        value = vm.pop
        ary = vm.peek
        ary.push(value)
      end
    end
  end
end
