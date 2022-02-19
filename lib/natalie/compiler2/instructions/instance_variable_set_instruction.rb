require_relative './base_instruction'

module Natalie
  class Compiler2
    class InstanceVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "instance_variable_set #{@name}"
      end

      def generate(transform)
        value = transform.pop
        transform.exec("self->ivar_set(env, #{@name.to_s.inspect}_s, #{value})")
      end

      def execute(vm)
        vm.self.instance_variable_set(@name, vm.pop)
      end
    end
  end
end
