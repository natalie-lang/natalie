require_relative './base_instruction'

module Natalie
  class Compiler2
    class GlobalVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_get #{@name}"
      end

      def generate(transform)
        transform.push("env->global_get(#{@name.to_s.inspect}_s)")
      end

      def execute(vm)
        vm.push(vm.global_variables[@name])
      end
    end
  end
end
