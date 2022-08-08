require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstSetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_s
      end

      attr_reader :name

      def to_s
        "const_set #{@name}"
      end

      def generate(transform)
        namespace = transform.pop
        value = transform.pop
        transform.exec_and_push(:const_set, "#{namespace}->const_set(#{transform.intern(@name)}, #{value})")
      end

      def execute(vm)
        namespace = vm.pop
        namespace = namespace.class unless namespace.is_a? Module
        value = vm.pop
        vm.push(namespace.const_set(@name, value))
      end
    end
  end
end
