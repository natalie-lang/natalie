require_relative './base_instruction'

module Natalie
  class Compiler
    class MatchBreakPointInstruction < BaseInstruction
      def initialize(break_point)
        @break_point = break_point
      end

      attr_reader :break_point

      def to_s
        "match_break_point #{@break_point}"
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_match_break(ptr %env, ptr %exception, i64 #{transform.break_point(@break_point)})",
        )
      end
    end
  end
end
