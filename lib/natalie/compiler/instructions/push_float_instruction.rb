require_relative './base_instruction'

module Natalie
  class Compiler
    class PushFloatInstruction < BaseInstruction
      def initialize(float)
        @float = float
      end

      attr_reader :float

      def to_s
        "push_float #{@float}"
      end

      def generate(transform)
        transform.push("Value(new FloatObject(#{@float}))")
      end

      def execute(vm)
        vm.push(@float)
      end
    end
  end
end
