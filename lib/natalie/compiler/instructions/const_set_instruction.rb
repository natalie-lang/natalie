require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstSetInstruction < BaseInstruction
      def initialize(name, strict:)
        @name = name.to_sym
        @strict = strict
      end

      attr_reader :name

      def to_s
        s = "const_set #{@name}"
        s << ' (strict)' if @strict
        s
      end

      def generate(transform)
        namespace = transform.pop
        value = transform.pop
        if @strict
          transform.exec("Object::const_set(env, #{namespace}, #{transform.intern(@name)}, #{value})")
        else
          transform.exec("Object::const_set_not_strict(env, #{transform.intern(@name)}, #{value})")
        end
      end

      def execute(vm)
        namespace = vm.pop
        namespace = namespace.class unless namespace.is_a? Module
        value = vm.pop
        namespace.const_set(@name, value)
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)
        [instruction_number, position, @strict ? 1 : 0].pack('CwC')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        strict = io.getbool
        new(name, strict: strict)
      end
    end
  end
end
