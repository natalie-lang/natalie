require_relative './base_instruction'

module Natalie
  class Compiler
    class PushSymbolInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "push_symbol #{@name.inspect}"
      end

      def generate(transform)
        sym = transform.intern(@name)
        transform.push("Value(#{sym})")
      end

      def execute(vm)
        vm.push(@name)
      end

      def serialize(rodata)
        position = rodata.add(name.to_s)

        [instruction_number, position].pack('Cw')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        new(name)
      end
    end
  end
end
