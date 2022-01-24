require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "variable_get #{@name}"
      end

      def generate(transform)
        var = transform.vars[@name]
        raise "unknown variable #{@name}" if var.nil?

        if var[:captured]
          index = var[:index]
          transform.push("env->var_get(#{@name.to_s.inspect}, #{index})")
        else
          transform.push(var[:name])
        end
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
