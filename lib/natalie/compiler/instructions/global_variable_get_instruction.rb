require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_get #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(:gvar, "env->global_get(#{transform.intern(@name)})")
      end

      def execute(vm)
        vm.push(vm.global_variables[@name])
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
