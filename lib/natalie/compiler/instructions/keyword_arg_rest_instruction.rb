require_relative './base_instruction'

module Natalie
  class Compiler
    class KeywordArgRestInstruction < BaseInstruction
      def initialize(without:)
        @without = without
      end

      def to_s
        "keyword_arg_rest without: #{@without.join(', ')}"
      end

      def generate(transform)
        transform.exec_and_push(
          'kwarg_rest',
          "args.keyword_arg_rest(env, { #{@without.map { |n| transform.intern(n) }.join(', ')} })",
        )
      end

      def execute(vm)
        hash = vm.kwargs.except(*@without)
        vm.push(hash)
      end

      def serialize(rodata)
        bytecode = [instruction_number, @without.size].pack('Cw')
        @without.each do |name|
          position = rodata.add(name)
          bytecode << [position].pack('w')
        end
        bytecode
      end

      def self.deserialize(io, rodata)
        without = []
        io.read_ber_integer.times do
          position = io.read_ber_integer
          without << rodata.get(position, convert: :to_sym)
        end
        new(without:)
      end
    end
  end
end
