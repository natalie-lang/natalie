require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "keyword_arg_get #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(@name, "args.keyword_arg(env, #{transform.intern(@name)})")
      end

      def execute(vm)
        hash = vm.kwargs
        vm.push(hash.fetch(@name))
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
