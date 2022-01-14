require_relative './base_instruction'

module Natalie
  class Compiler2
    class ConstFindInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_s
      end

      attr_reader :name

      def to_s
        "const_find #{@name}"
      end

      def to_cpp(transform)
        "self->const_find(env, #{name.inspect}_s, Object::ConstLookupSearchMode::NotStrict)"
      end

      def execute(vm)
        vm.push Object.const_get(@name)
      end
    end
  end
end
