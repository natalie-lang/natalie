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

      def serialize(_)
        name_string = @name.to_s
        [
          instruction_number,
          name_string.bytesize,
          name_string,
          @local_only ? 1 : 0,
        ].pack("Cwa#{name_string.bytesize}C")
      end

      def self.deserialize(io, _)
        size = io.read_ber_integer
        name = io.read(size).to_sym
        local_only = io.getbool
        new(name, local_only: local_only)
      end
    end
  end
end
