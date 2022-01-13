require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name
      end

      attr_reader :name

      def to_s
        "variable_set #{@name}"
      end

      def to_cpp(transform)
        # FIXME: this is overkill -- there are variables not captured in this count, i.e. "holes" in the array :-(
        index = transform.vars.size

        # FIXME: not all variables need to be captured
        transform.vars[name] = { name: name, index: index, captured: true }

        value = transform.pop
        transform.push "env->var_set(#{name.to_s.inspect}, #{index}, true, #{value})"
        value
      end

      def execute(vm)
        vm.vars[@name] = vm.pop
      end
    end
  end
end
