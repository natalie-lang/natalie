require_relative './base_instruction'

module Natalie
  class Compiler
    class ClassVariableGetInstruction < BaseInstruction
      def initialize(name, default_to_nil: false)
        @name = name.to_sym
        @default_to_nil = default_to_nil
      end

      attr_reader :name

      def to_s
        s = "class_variable_get #{@name}"
        s << ' (default_to_nil)' if @default_to_nil
        s
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_cvar_get(ptr %env, i64 %self, ptr #{transform.string(@name.to_s)}, i32 #{@default_to_nil ? 1 : 0})",
        )
      end

      def generate(transform)
        if @default_to_nil
          transform.exec_and_push(:cvar, "self->cvar_get_maybe(env, #{transform.intern(@name)}).value_or(Value::nil())")
        else
          transform.exec_and_push(:cvar, "self->cvar_get(env, #{transform.intern(@name)})")
        end
      end
    end
  end
end
