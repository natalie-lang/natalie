require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatSetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_set'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("Value anon_splat = #{value}")
      end

      def execute(vm)
        vm.scope[:vars][:anon_splat] = { value: vm.pop }
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
