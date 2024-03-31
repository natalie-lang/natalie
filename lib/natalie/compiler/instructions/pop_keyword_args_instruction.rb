require_relative './base_instruction'

module Natalie
  class Compiler
    class PopKeywordArgsInstruction < BaseInstruction
      def to_s
        'pop_keyword_args'
      end

      def generate(transform)
        transform.exec_and_push(:keyword_args_hash, 'args.has_keyword_hash() ? args.pop_keyword_hash() : new HashObject')
      end

      def execute(vm)
        if vm.args.empty? || !vm.args.last.is_a?(Hash)
          vm.push Hash.new
        else
          vm.push vm.args.pop
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
