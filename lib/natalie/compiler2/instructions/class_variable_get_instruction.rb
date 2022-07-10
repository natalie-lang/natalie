require_relative './base_instruction'

module Natalie
  class Compiler2
    class ClassVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "class_variable_get #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(:ivar, "self->cvar_get(env, #{@name.to_s.inspect}_s)")
      end

      def execute(vm)
        vm.push(vm.self.class.class_variable_get(@name))
      end
    end
  end
end
