require_relative './base_instruction'

module Natalie
  class Compiler2
    class GlobalVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_set #{@name}"
      end

      def generate(transform)
        value = transform.pop
        transform.exec("env->global_set(#{@name.to_s.inspect}_s, #{value})")
      end

      def execute(vm)
        vm.global_variables[@name] = vm.pop
      end
    end
  end
end
