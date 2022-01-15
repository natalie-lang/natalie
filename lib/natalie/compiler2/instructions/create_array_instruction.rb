require_relative './base_instruction'

module Natalie
  class Compiler2
    class CreateArrayInstruction < BaseInstruction
      def initialize(count:)
        @count = count
      end

      def to_s
        "create_array #{@count}"
      end

      def generate(transform)
        items = []
        @count.times { items.unshift(transform.pop) }
        transform.push("(new ArrayObject({ #{items.join(', ')} }))")
      end

      def execute(vm)
        ary = []
        @count.times { ary.unshift(vm.pop) }
        vm.push(ary)
      end
    end
  end
end
