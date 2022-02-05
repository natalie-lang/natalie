require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushRangeInstruction < BaseInstruction
      def initialize(exclude_end)
        @exclude_end = exclude_end
      end

      def to_s
        s = 'push_range'
        s << ' (exclude end)' if @exclude_end
        s
      end

      def generate(transform)
        beginning = transform.pop
        ending = transform.pop
        transform.push("Value(new RangeObject(#{beginning}, #{ending}, #{@exclude_end}))")
      end

      def execute(vm)
        beginning = vm.pop
        ending = vm.pop
        if @exclude_end
          vm.push(beginning...ending)
        else
          vm.push(beginning..ending)
        end
      end
    end
  end
end
