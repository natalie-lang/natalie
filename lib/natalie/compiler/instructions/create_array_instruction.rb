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
          transform.exec_and_push(:array, "Value(new ArrayObject({ #{items.join(', ')} }))")
        end
      end

      def execute(vm)
        ary = []
        @count.times { ary.unshift(vm.pop) }
        vm.push(ary)
      end
    end
  end
end
