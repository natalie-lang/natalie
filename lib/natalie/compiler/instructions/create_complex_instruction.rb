require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateComplexInstruction < BaseInstruction
      def to_s
        'create_complex'
      end

      def generate(transform)
        imaginary = transform.pop
        transform.exec_and_push(:complex, "Value(new ComplexObject(Value::integer(0), #{imaginary}))")
      end

      def execute(vm)
        imaginary = vm.pop
        
        vm.push(Complex(0, imaginary))
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
