require_relative './base_instruction'

module Natalie
  class Compiler
    class ClassVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "class_variable_set #{@name}"
      end

      def generate(transform)
        value = transform.pop
        transform.exec("self->cvar_set(env, #{transform.intern(@name)}, #{value})")
      end
    end
  end
end
