require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousBlockGetInstruction < BaseInstruction
      def to_s
        'anonymous_block_get'
      end

      def generate(transform)
        transform.push('anon_block')
      end

      def execute(vm)
        if (var = vm.find_var(:anon_block))
          vm.push(var.fetch(:value))
        else
          raise "anonymous block was not defined #{@name}"
        end
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
