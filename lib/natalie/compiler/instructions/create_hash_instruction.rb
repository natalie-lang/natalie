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
        transform.exec_and_push(:hash, "Value(HashObject::create(env, #{items.size}, #{items_temp}))")
      end
    end
  end
end
