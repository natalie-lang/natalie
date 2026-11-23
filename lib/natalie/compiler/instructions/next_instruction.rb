require_relative './base_instruction'

module Natalie
  class Compiler
    class NextInstruction < BaseInstruction
      attr_accessor :jump_point

      def to_s
        'next'
      end

      def generate_llvm(transform)
        transform.return_value(transform.pop)
      end

      def generate(transform)
        if @jump_point
          value = transform.pop
          transform.exec("env->raise_local_jump_error(#{value}, LocalJumpErrorType::Return, #{@jump_point})")
          transform.push_nil
          return
        end
        transform.exec("return #{transform.pop}")
        transform.push_nil
      end
    end
  end
end
