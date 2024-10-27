require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_splat_get'
      end

      def generate(transform)
        transform.push('anon_splat')
      end

      def execute(vm)
        if (var = vm.find_var(:anon_splat))
          vm.push(var.fetch(:value))
        else
          raise "anonymous splat was not defined #{@name}"
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
