require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateArrayInstruction < BaseInstruction
      def initialize(count:)
        @count = count
      end

      def to_s
        "create_array #{@count}"
      end

      def generate(transform)
        items = []
        if @count.zero?
          transform.exec_and_push(:array, "Value(new ArrayObject)")
        else
          @count.times { items.unshift(transform.pop) }
          items_temp = transform.temp('items')
          transform.exec("Value #{items_temp}[] = { #{items.join(', ')} };")
          transform.exec_and_push(:array, "Value(new ArrayObject(#{@count}, #{items_temp}))")
        end
      end

      def execute(vm)
        ary = []
        @count.times { ary.unshift(vm.pop) }
        vm.push(ary)
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
