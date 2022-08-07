require_relative './base_instruction'

module Natalie
  class Compiler2
    class InstanceVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "instance_variable_get #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(:ivar, "self->ivar_get(env, #{transform.intern(@name)})")
      end

      def execute(vm)
        vm.push(vm.self.instance_variable_get(@name))
      end
    end
  end
end
