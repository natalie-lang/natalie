require_relative './base_instruction'

module Natalie
  class Compiler
    class AnonymousKeywordSplatGetInstruction < BaseInstruction
      def to_s
        'anonymous_keyword_splat_get'
      end

      def generate(transform)
        transform.push('anon_keyword_splat')
      end

      def execute(vm)
        if (var = vm.find_var(:anon_keyword_splat))
          vm.push(var.fetch(:value))
        else
          raise "anonymous keyword splat was not defined #{@name}"
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
