require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousBlockSetInstruction < BaseInstruction
      def to_s
        'anonymous_block_set'
      end

      def generate(transform)
        value = transform.pop
        transform.exec("Value anon_block = #{value}")
      end

      def execute(vm)
        vm.scope[:vars][:anon_block] = { value: vm.pop }
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
