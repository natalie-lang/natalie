require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgInstruction < BaseInstruction
      def initialize(index)
        @index = index
      end

      attr_reader :index

      def to_s
        "push_arg #{@index}"
      end

      def generate(transform)
        transform.push("args[#{@index}]")
      end

      def execute(vm)
        vm.push(vm.args[@index])
      end
    end
  end
end
