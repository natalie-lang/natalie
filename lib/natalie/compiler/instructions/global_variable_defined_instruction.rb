require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableDefinedInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_defined #{@name}"
      end

      def generate(transform)
        transform.exec("if (!env->global_defined(#{transform.intern(@name)})) throw new ExceptionObject")
        transform.push_nil
      end

      def execute(vm)
        vm.push(vm.global_variables.key?(@name))
      end
    end
  end
end
