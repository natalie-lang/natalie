require_relative './base_instruction'

module Natalie
  class Compiler
    class SingletonClassInstruction < BaseInstruction
      def to_s
        'singleton_class'
      end

      def generate(transform)
        obj = transform.pop
        transform.push("#{obj}->singleton_class(env)")
      end

      def execute(vm)
        obj = vm.pop
        vm.push(obj.singleton_class)
      end
    end
  end
end
