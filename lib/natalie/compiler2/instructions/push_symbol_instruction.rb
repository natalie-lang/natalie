require_relative './base_instruction'
require_relative './string_to_cpp'

module Natalie
  class Compiler2
    class PushSymbolInstruction < BaseInstruction
      include StringToCpp

      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "push_symbol #{@name.inspect}"
      end

      def generate(transform)
        transform.push("Value(#{string_to_cpp(@name.to_s)}_s)")
      end

      def execute(vm)
        vm.push(@name)
      end
    end
  end
end
