require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsShiftInstruction < BaseInstruction
      def to_s
        'args_shift'
      end

      def generate(transform)
        transform.exec_and_push(:first_arg, 'args.shift()')
      end

      def execute(vm)
        vm.push(vm.args.shift)
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
