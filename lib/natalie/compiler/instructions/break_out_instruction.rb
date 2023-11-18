require_relative './base_instruction'

module Natalie
  class Compiler
    # This instruction is really just used for breaking from a `while` loop.
    class BreakOutInstruction < BaseInstruction
      def to_s
        'break_out'
      end

      def generate(transform)
        value = transform.pop
        while_env = @env
        while_env = while_env.fetch(:outer) until while_env[:while]
        result_name = while_env.fetch(:result_name)
        transform.exec("#{result_name} = #{value}")
        transform.exec("break")
      end

      def execute(vm)
        :break_out
      end
    end
  end
end
