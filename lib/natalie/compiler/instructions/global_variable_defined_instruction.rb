require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableDefinedInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_defined #{@name}"
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_global_defined(ptr %env, ptr #{transform.string(@name.to_s)})"
      end
    end
  end
end
