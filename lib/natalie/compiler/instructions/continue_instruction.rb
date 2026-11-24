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
    end
  end
end
