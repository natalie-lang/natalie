require_relative './base_instruction'

module Natalie
  class Compiler
    class PushComplexInstruction < BaseInstruction
      def to_s
        'push_complex'
      end

      def generate(transform)
        imaginary = transform.pop
        real = transform.pop
        transform.exec_and_push(:range, "Value(new ComplexObject(#{real}, #{imaginary}))")
      end

      def execute(vm)
        imaginary = vm.pop
        real = vm.pop
        
        vm.push(Complex(real, imaginary))
      end
    end
  end
end
