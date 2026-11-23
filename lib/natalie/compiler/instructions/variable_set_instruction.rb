# frozen_string_literal: true

require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableSetInstruction < BaseInstruction
      def initialize(name, local_only: false)
        @name = name.to_sym
        @local_only = local_only
      end

      attr_reader :name, :local_only

      attr_accessor :meta

      def to_s
        s = "variable_set #{@name}"
        s << ' local' if @local_only
        s
      end

      def generate_llvm(transform)
        depth, index = transform.variable_address(self)
        transform.emit "call void @nat_llvm_var_set(ptr %env, i64 #{depth}, i64 #{index}, i64 #{transform.pop})"
      end

      def generate(transform)
        depth, var = transform.find_var(name, local_only: @local_only)
        index = var.fetch(:index)

        env = +'env'
        depth.times { env << '->outer()' }

        value = transform.pop

        if @meta.fetch(:captured)
          @meta[:declared] = true
          transform.exec("#{env}->var_set(#{name.to_s.inspect}, #{index}, true, #{value})")
        elsif @meta.fetch(:declared)
          transform.exec("#{@meta[:name]} = #{value}")
        else
          @meta[:declared] = true
          transform.exec("Value #{@meta[:name]} = #{value}")
        end
      end
    end
  end
end
