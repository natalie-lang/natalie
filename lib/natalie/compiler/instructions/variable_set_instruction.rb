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

      def generate(transform)
        ((depth, var) = transform.find_var(name, local_only: @local_only))
        index = var.fetch(:index)

        env = 'env'
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

      def execute(vm)
        if (var = vm.find_var(@name, local_only: @local_only))
          var[:value] = vm.pop
        else
          vm.scope[:vars][@name] = { name: @name, value: vm.pop }
        end
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)
        [
          instruction_number,
          position,
          @local_only ? 1 : 0,
        ].pack('CwC')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        local_only = io.getbool
        new(name, local_only: local_only)
      end
    end
  end
end
