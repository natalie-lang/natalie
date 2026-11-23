require_relative './base_instruction'

module Natalie
  class Compiler
    # This instruction is really just used for skipping one iteration in a `while` loop.
    class ContinueInstruction < BaseInstruction
      attr_accessor :jump_point

      def to_s
        'continue'
      end

      def generate_llvm(transform)
        transform.loop_jump(:next)
      end

      def generate(transform)
        if @jump_point
          value = 'Value::nil()'
          transform.exec("env->raise_local_jump_error(#{value}, LocalJumpErrorType::Return, #{@jump_point})")
          transform.push_nil
          return
        end
        transform.exec('continue')
        transform.push_nil
      end
    end
  end
end
