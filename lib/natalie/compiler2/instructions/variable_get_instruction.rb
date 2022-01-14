require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_s
      end

      attr_reader :name

      def to_s
        "variable_get #{@name}"
      end

      def to_cpp(transform)
        var = transform.vars[@name]
        raise "unknown variable #{@name}" if var.nil?

        index = var[:index]
        "env->var_get(#{@name.to_s.inspect}, #{index})"
      end

      def execute(vm)
        if vm.vars.key?(@name)
          vm.push(vm.vars[@name])
        else
          raise "unknown variable #{@name}"
        end
      end
    end
  end
end
