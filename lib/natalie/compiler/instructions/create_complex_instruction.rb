require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateComplexInstruction < BaseInstruction
      def to_s
        'create_complex'
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

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
