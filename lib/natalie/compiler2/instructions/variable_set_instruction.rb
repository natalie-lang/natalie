require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "variable_set #{@name}"
      end

      def generate(transform)
        value = transform.pop
        if (var = transform.vars[name])
          index = var[:index]
        else
          # FIXME: this is overkill -- there are variables not captured in this count, i.e. "holes" in the array :-(
          index = transform.vars.size

          # TODO: not all variables need to be captured
          transform.vars[name] = { name: name, index: index, captured: true }
        end
        if (index == nil)
          throw "Not indexed variable #{name} is captured" if var[:captured]
          transform.exec("#{var[:name]} = #{value}")
        else
          transform.exec("env->var_set(#{name.to_s.inspect}, #{index}, true, #{value})")
        end
      end

      def execute(vm)
        vm.vars[@name] = vm.pop
      end
    end
  end
end
