require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushSymbolInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "push_symbol #{@name.inspect}"
      end

      def generate(transform)
        transform.push("\"#{@name.to_s}\"_s")
      end

      def execute(vm)
        vm.push(@name)
      end
    end
  end
end
