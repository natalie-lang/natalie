require_relative './base_instruction'

module Natalie
  class Compiler2
    class MemoizeInstruction < BaseInstruction
      def initialize(name)
        @name = name
      end

      def to_s
        "memoize #{@name}"
      end

      def generate(transform)
        transform.exec_and_push(@name, transform.pop)
      end

      def execute(vm)
        # nothing to do
      end
    end
  end
end
