require_relative './base_instruction'

module Natalie
  class Compiler
    class CaseEqualInstruction < BaseInstruction
      def initialize(is_splat:)
        @is_splat = is_splat
      end

      attr_reader :is_splat

      def to_s
        s = 'case_equal'
        s << ' (is splat)' if @is_splat
        s
      end

      def generate_llvm(transform)
        when_value = transform.pop
        transform.push_call(
          "call i64 @nat_llvm_case_equal(ptr %env, i64 #{transform.peek}, i64 #{when_value}, i32 #{@is_splat ? 1 : 0})",
        )
      end
    end
  end
end
