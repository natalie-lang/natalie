require_relative './base_instruction'

module Natalie
  class Compiler
    class IsNilInstruction < BaseInstruction
      def to_s
        'is_nil'
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:is_nil, "bool_object(#{obj}.is_nil())")
      end

      def execute(vm)
        obj = vm.pop
        vm.push obj.nil?
      end
    end
  end
end
