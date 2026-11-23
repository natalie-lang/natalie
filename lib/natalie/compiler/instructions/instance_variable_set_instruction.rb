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

      def generate(transform)
        value = transform.pop
        transform.exec("Object::ivar_set(env, self, #{transform.intern(@name)}, #{value})")
      end
    end
  end
end
