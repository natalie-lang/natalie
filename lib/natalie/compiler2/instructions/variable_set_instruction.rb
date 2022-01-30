require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableSetInstruction < BaseInstruction
      def initialize(name, local_only: false)
        @name = name.to_sym
        @local_only = false
        @local_only = local_only
      end

      attr_reader :name

      def to_s
        s = "variable_set #{@name}"
        s << ' local' if @local_only
        s
      end

      def generate(transform)
        if ((depth, var) = transform.find_var(name, local_only: @local_only))
          index = var[:index]
        else
          # FIXME: this is overkill -- there are variables not captured in this count, i.e. "holes" in the array :-(
          index = transform.vars.size

          # TODO: not all variables need to be captured
          transform.vars[name] = { name: name, index: index, captured: true }

          depth = 0
        end

        env = 'env'
        depth.times { env << '->outer()' }

        value = transform.pop

        transform.exec("#{env}->var_set(#{name.to_s.inspect}, #{index}, true, #{value})")
      end

      def execute(vm)
        if (var = vm.find_var(@name, local_only: @local_only))
          var[:value] = vm.pop
        else
          vm.scope[:vars][@name] = { name: @name, value: vm.pop }
        end
      end
    end
  end
end
