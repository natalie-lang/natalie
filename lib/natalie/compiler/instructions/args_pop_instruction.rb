require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsPopInstruction < BaseInstruction
      def to_s
        'args_pop'
      end

      def generate(transform)
        transform.exec_and_push(:last_arg, 'args.pop()')
      end

      def execute(vm)
        vm.push(vm.args.pop)
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
