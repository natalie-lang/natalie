require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgPresentInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "keyword_arg_present #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(:has_key, "bool_object(args.keyword_arg_present(env, #{transform.intern(@name)}))")
      end

      def execute(vm)
        vm.push(vm.args.last&.key?(@key))
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
