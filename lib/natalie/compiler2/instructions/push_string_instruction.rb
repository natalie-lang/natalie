require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushStringInstruction < BaseInstruction
      def initialize(string)
        @string = string
      end

      def to_s
        "push_string #{@string.inspect}"
      end

      def to_cpp(transform)
        "new StringObject(#{@string.inspect})"
      end

      def execute(vm)
        vm.push(@string)
      end
    end
  end
end
