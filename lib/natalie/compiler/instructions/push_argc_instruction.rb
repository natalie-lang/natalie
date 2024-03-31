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

      def serialize(_)
        [
          instruction_number,
          @count
        ].pack("Cw")
      end

      def self.deserialize(io, _)
        count = io.read_ber_integer
        new(count)
      end
    end
  end
end
