require_relative './base_instruction'

module Natalie
  class Compiler
    # This instruction is really just used for breaking from a `while` loop.
    class BreakOutInstruction < BaseInstruction
      attr_accessor :jump_point

      def to_s
        'break_out'
      end

      def generate_llvm(transform)
        transform.loop_jump(:break, transform.pop)
      end

      def generate(transform)
        if @jump_point
          value = transform.pop
          transform.exec("env->raise_local_jump_error(#{value}, LocalJumpErrorType::Return, #{@jump_point})")
          transform.push_nil
          return
        end
        value = transform.pop
        while_env = @env
        while_env = while_env.fetch(:outer) until while_env.fetch(:type) == :while
        result_name = while_env.fetch(:result_name)
        transform.exec("#{result_name} = #{value}")
        transform.exec('break')
        transform.push_nil
      end
    end
  end
end
