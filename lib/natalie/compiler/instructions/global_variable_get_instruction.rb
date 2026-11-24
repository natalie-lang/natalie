require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_get #{@name}"
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_global_get(ptr %env, ptr #{transform.string(@name.to_s)})")
      end
    end
  end
end
