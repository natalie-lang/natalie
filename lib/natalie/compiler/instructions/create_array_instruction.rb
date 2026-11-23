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
          transform.exec_and_push(:array, 'Value(ArrayObject::create())')
        else
          @count.times { items.unshift(transform.pop) }
          items_temp = transform.temp('items')
          transform.exec("Value #{items_temp}[] = { #{items.join(', ')} };")
          transform.exec_and_push(:array, "Value(ArrayObject::create(#{@count}, #{items_temp}))")
        end
      end
    end
  end
end
