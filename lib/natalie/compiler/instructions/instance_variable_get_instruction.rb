require_relative './base_instruction'

module Natalie
  class Compiler
    class InstanceVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "instance_variable_get #{@name}"
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_ivar_get(ptr %env, i64 %self, ptr #{transform.string(@name.to_s)})")
      end
    end
  end
end
