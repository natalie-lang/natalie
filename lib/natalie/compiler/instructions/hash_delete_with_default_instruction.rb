require_relative './base_instruction'

module Natalie
  class Compiler
    class HashDeleteWithDefaultInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "hash_delete_with_default #{@name}"
      end

      def generate(transform)
        default = transform.pop
        hash = transform.memoize(:hash, transform.peek)
        name_sym = transform.intern(@name)
        code = "(#{hash}->as_hash()->has_key(env, #{name_sym}) ? #{hash}->as_hash()->remove(env, #{name_sym}) : #{default})"
        transform.exec_and_push(:value_removed_from_hash, code)
      end

      def execute(vm)
        default = vm.pop
        hash = vm.peek
        vm.push(hash.key?(@name) ? hash.delete(@name) : default)
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
