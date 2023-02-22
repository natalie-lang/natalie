require_relative './base_instruction'

module Natalie
  class Compiler
    class PushBlockInstruction < BaseInstruction
      def initialize(from_nearest_env: false)
        @from_nearest_env = from_nearest_env
      end

      def to_s
        s = 'push_block'
        s << ' (from_nearest_env)' if @from_nearest_env
        s
      end

      def generate(transform)
        if @from_nearest_env
          transform.exec_and_push(:block, "ProcObject::from_block_maybe(env->nearest_block(true))")
        else
          transform.exec_and_push(:block, "ProcObject::from_block_maybe(block)")
        end
      end

      def execute(vm)
        vm.push(vm.block)
      end
    end
  end
end
