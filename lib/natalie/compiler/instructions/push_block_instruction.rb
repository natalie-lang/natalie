require_relative './base_instruction'

module Natalie
  class Compiler
    class PushBlockInstruction < BaseInstruction
      def to_s
        'push_block'
      end

      def generate(transform)
        transform.exec_and_push(:block, "ProcObject::from_block_maybe(block)")
      end

      def execute(vm)
        vm.push(vm.block)
      end
    end
  end
end
