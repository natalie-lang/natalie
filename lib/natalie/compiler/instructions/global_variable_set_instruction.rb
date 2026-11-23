require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_set #{@name}"
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_global_set(ptr %env, ptr #{transform.string(@name.to_s)}, i64 #{transform.pop})"
      end

      def generate(transform)
        value = transform.pop
        transform.exec("env->global_set(#{transform.intern(@name)}, #{value})")
      end
    end
  end
end
