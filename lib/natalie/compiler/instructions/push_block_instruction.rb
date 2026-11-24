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

      def generate_llvm(transform)
        block = transform.function_block
        transform.push_call("call i64 @nat_llvm_push_block(ptr %env, ptr #{block}, i32 #{@from_nearest_env ? 1 : 0})")
      end
    end
  end
end
