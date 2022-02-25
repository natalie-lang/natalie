require_relative './base_instruction'

module Natalie
  class Compiler2
    class MatchBreakPointInstruction < BaseInstruction
      def initialize(break_point)
        @break_point = break_point
      end

      attr_reader :break_point

      def to_s
        "match_break_point #{@break_point}"
      end

      def generate(transform)
        code = "exception->is_local_jump_error_with_break_point(env, #{@break_point})"
        transform.exec_and_push(:match_exception_result, code)
      end

      def execute(vm)
        exception = vm.global_variables[:$!]
        vm.push(exception.instance_variable_get(:@break_point) == @break_point)
      end
    end
  end
end
