require_relative './base_instruction'

module Natalie
  class Compiler2
    class VariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      attr_accessor :meta

      def to_s
        "variable_get #{@name}"
      end

      def generate(transform)
        (depth, var) = transform.find_var(@name)
        raise "unknown variable #{@name}" if var.nil?

        env = 'env'
        depth.times { env << '->outer()' }
        index = var[:index]

        if @meta.fetch(:captured)
          transform.push("#{env}->var_get(#{@name.to_s.inspect}, #{index})")
        else
          transform.push(@meta.fetch(:name))
        end
      end

      def execute(vm)
        if (var = vm.find_var(@name))
          vm.push(var.fetch(:value))
        else
          raise "unknown variable #{@name}"
        end
      end
    end
  end
end
