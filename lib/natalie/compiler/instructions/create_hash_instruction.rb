require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateHashInstruction < BaseInstruction
      def initialize(count:)
        @count = count
      end

      attr_reader :count

      def to_s
        "create_hash count: #{@count}"
      end

      def generate(transform)
        items = []
        @count.times do
          items.unshift(transform.pop)
          items.unshift(transform.pop)
        end
        items_temp = transform.temp('items')
        transform.exec("Value #{items_temp}[] = { #{items.join(', ')} };")
        transform.exec_and_push(:hash, "Value(new HashObject(env, #{items.size}, #{items_temp}))")
      end

      def execute(vm)
        items = []
        @count.times do
          items.unshift(vm.pop)
          items.unshift(vm.pop)
        end
        hash = Hash[items.each_slice(2).to_a]
        vm.push(hash)
      end

      def serialize(_)
        [
          instruction_number,
          @count,
        ].pack('Cw')
      end

      def self.deserialize(io, _)
        count = io.read_ber_integer
        new(count:)
      end
    end
  end
end
