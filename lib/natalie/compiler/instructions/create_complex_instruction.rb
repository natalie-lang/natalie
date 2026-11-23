require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateComplexInstruction < BaseInstruction
      def to_s
        'create_complex'
      end

      def generate_llvm(transform)
        imaginary = transform.pop
        transform.push_call("call i64 @nat_llvm_complex(i64 #{imaginary})")
      end

      def generate(transform)
        imaginary = transform.pop
        transform.exec_and_push(:complex, "Value(ComplexObject::create(Value::integer(0), #{imaginary}))")
      end
    end
  end
end
