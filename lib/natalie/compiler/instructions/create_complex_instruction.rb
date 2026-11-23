require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateComplexInstruction < BaseInstruction
      def to_s
        'create_complex'
      end

      def generate(transform)
        imaginary = transform.pop
        transform.exec_and_push(:complex, "Value(ComplexObject::create(Value::integer(0), #{imaginary}))")
      end
    end
  end
end
