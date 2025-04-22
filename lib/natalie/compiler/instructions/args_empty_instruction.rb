require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsEmptyInstruction < BaseInstruction
      def to_s
        'args_empty'
      end

      def generate(transform)
        transform.exec_and_push(:args_empty, 'bool_object(args.size(false) == 0)')
      end

      def execute(vm)
        raise 'todo'
        vm.push(vm.args.last&.key?(@key))
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
