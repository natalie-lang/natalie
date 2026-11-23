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

      def generate(transform)
        transform.exec_and_push(:ivar, "Object::ivar_get(env, self, #{transform.intern(@name)})")
      end
    end
  end
end
