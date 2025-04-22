require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgDeleteInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "keyword_arg_delete #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(@name, "args.keyword_hash()->remove(env, #{transform.intern(@name)}).value()")
      end

      def execute(vm)
        hash = vm.args.last
        vm.push(hash.delete(@name))
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)
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
