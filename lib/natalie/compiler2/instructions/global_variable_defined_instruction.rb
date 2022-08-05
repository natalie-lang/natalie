require_relative './base_instruction'

module Natalie
  class Compiler2
    class GlobalVariableDefinedInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_defined #{@name}"
      end

      def generate(transform)
        transform.exec("if (!env->global_defined(#{@name.to_s.inspect}_s)) throw new ExceptionObject")
        transform.push('NilObject::the()')
      end

      def execute(vm)
        vm.push(vm.global_variables.key?(@name))
      end
    end
  end
end
