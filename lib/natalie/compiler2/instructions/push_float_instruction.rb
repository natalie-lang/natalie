require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushFloatInstruction < BaseInstruction
      def initialize(float)
        @float = float
      end

      attr_reader :float

      def to_s
        "push_float #{@float}"
      end

      def to_cpp(transform)
        "new FloatValue { #{@float} }"
      end

      def execute(vm)
        vm.push(@float)
      end
    end
  end
end
