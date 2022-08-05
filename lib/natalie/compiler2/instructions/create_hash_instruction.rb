require_relative './base_instruction'

module Natalie
  class Compiler2
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
        transform.exec_and_push(:hash, "Value(new HashObject(env, { #{items.join(', ')} }))")
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
