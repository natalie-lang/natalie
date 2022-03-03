require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushStringInstruction < BaseInstruction
      def initialize(string, length)
        @string = string
        @length = length
      end

      def to_s
        "push_string #{@string.inspect}, #{@length}"
      end

      def generate(transform)
        transform.exec_and_push('string', "Value(new StringObject(#{@string.inspect}, #{@length}))")
      end

      def execute(vm)
        vm.push(@string.dup)
      end
    end
  end
end
