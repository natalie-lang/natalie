require_relative './base_instruction'

module Natalie
  class Compiler
    class InstanceVariableDefinedInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "instance_variable_defined #{@name}"
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_ivar_defined(ptr %env, i64 %self, ptr #{transform.string(@name.to_s)})"
      end

      def generate(transform)
        transform.exec(
          "if (!Object::ivar_defined(env, self, #{transform.intern(@name)})) throw ExceptionObject::create()",
        )
      end
    end
  end
end
