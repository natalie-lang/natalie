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
        transform.push("self->ivar_get(env, #{@name.to_s.inspect}_s)")
      end

      def execute(vm)
        vm.push(vm.self.instance_variable_get(@name))
      end
    end
  end
end
