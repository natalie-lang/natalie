require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateHashInstruction < BaseInstruction
      def initialize(count:, bare:)
        @count = count
        @bare = bare
      end

      attr_reader :count

      def bare?
        !!@bare
      end

      def to_s
        s = "create_hash count: #{@count}"
        s << ' (bare)' if @bare
        s
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
    end
  end
end
