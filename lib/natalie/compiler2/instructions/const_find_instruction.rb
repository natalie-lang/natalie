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

      def generate(transform)
        namespace = transform.pop
        transform.push("#{namespace}->const_find(env, #{name.inspect}_s, Object::ConstLookupSearchMode::NotStrict)")
      end

      def execute(vm)
        namespace = vm.pop
        namespace = namespace.class unless namespace.respond_to?(:const_get)
        vm.push namespace.const_get(@name)
      end
    end
  end
end
