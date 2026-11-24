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
    end
  end
end
