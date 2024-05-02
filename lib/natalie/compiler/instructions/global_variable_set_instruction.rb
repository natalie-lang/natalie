require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_set #{@name}"
      end

      def generate(transform)
        value = transform.pop
        transform.exec("env->global_set(#{transform.intern(@name)}, #{value})")
      end

      def execute(vm)
        vm.global_variables[@name] = vm.pop
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)
        [
          instruction_number,
          position,
        ].pack('Cw')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        new(name)
      end
    end
  end
end
