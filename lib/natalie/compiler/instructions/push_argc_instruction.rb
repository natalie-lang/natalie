require_relative './base_instruction'

module Natalie
  class Compiler
    class PushArgcInstruction < BaseInstruction
      def initialize(count)
        @count = count
      end

      attr_reader :count

      def to_s
        "push_argc #{@count}"
      end

      def generate(transform)
        transform.push(@count)
      end

      def execute(vm)
        vm.push(@count)
      end

      def serialize
        [
          instruction_number,
          @count
        ].pack("CS")
      end
    end
  end
end
