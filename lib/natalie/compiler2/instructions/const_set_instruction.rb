require_relative './base_instruction'

module Natalie
  class Compiler2
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
        transform.exec_and_push(:const_set, "#{namespace}->const_set(#{@name.inspect}_s, #{value})")
      end

      def execute(vm)
        namespace = vm.pop
        value = vm.pop
        if namespace.is_a? Natalie::VM::MainObject
          namespace = Object
        elsif !namespace.is_a? Module
          raise TypeError, "#{namespace} is not a class/module"
        end
        vm.push(namespace.const_set(@name, value))
      end
    end
  end
end
