require_relative './base_instruction'

module Natalie
  class Compiler
    class InstanceVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "instance_variable_set #{@name}"
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_ivar_set(ptr %env, i64 %self, ptr #{transform.string(@name.to_s)}, i64 #{transform.pop})"
      end
    end
  end
end
