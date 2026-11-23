require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableDeclareInstruction < BaseInstruction
      def initialize(name, local_only: false)
        @name = name.to_sym
        @local_only = local_only
      end

      attr_reader :name, :local_only

      attr_accessor :meta

      def to_s
        s = "variable_declare #{@name}"
        s << ' local' if @local_only
        s
      end

      def generate_llvm(transform)
        # Environment slots are initialized to nil at function entry.
      end

      def generate(transform)
        depth, var = transform.find_var(name, local_only: @local_only)
        index = var.fetch(:index)

        return if @meta.fetch(:declared)

        env = 'env'
        depth.times { env << '->outer()' }

        if @meta.fetch(:captured)
          @meta[:declared] = true
          transform.exec("#{env}->var_set(#{name.to_s.inspect}, #{index}, true, Value::nil())")
        else
          @meta[:declared] = true
          transform.exec("Value #{@meta[:name]} = Value::nil()")
        end
      end
    end
  end
end
