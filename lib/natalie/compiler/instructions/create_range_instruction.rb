require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateRangeInstruction < BaseInstruction
      def initialize(exclude_end)
        @exclude_end = exclude_end
      end

      def to_s
        s = 'create_range'
        s << ' (exclude end)' if @exclude_end
        s
      end

      def generate_llvm(transform)
        beginning, ending = transform.pop, transform.pop
        transform.push_call(
          "call i64 @nat_llvm_range(ptr %env, i64 #{beginning}, i64 #{ending}, i32 #{@exclude_end ? 1 : 0})",
        )
      end
    end
  end
end
