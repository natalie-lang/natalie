require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushIntInstruction < BaseInstruction
      def initialize(int)
        @int = int
      end

      attr_reader :int

      def to_s
        "push_int #{@int}"
      end

      def generate(transform)
        transform.push("Value::integer(#{@int})")
      end

      def execute(vm)
        vm.push(@int)
      end
    end
  end
end
