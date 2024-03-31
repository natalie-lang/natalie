require_relative './base_instruction'

module Natalie
  class Compiler
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
        transform.exec_and_push(:range, "Value(RangeObject::create(env, #{beginning}, #{ending}, #{@exclude_end}))")
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

      def serialize(_)
        name_string = @name.to_s
        [
          instruction_number,
          @exclude_end ? 1 : 0,
        ].pack('CC')
      end

      def self.deserialize(io, _)
        exclude_end = io.getbool
        new(exclude_end)
      end
    end
  end
end
