require_relative './base_instruction'

module Natalie
  class Compiler
    class PushSymbolInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "push_symbol #{@name.inspect}"
      end

      def generate(transform)
        sym = transform.intern(@name)
        transform.push("Value(#{sym})")
      end

      def execute(vm)
        vm.push(@name)
      end
    end
  end
end
