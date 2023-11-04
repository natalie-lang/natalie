require_relative './base_instruction'

module Natalie
  class Compiler
    # This instruction is really just used for skipping one iteration in a `while` loop.
    class ContinueInstruction < BaseInstruction
      def to_s
        'continue'
      end

      def generate(transform)
        transform.exec('continue')
        transform.push_nil
      end

      def execute(vm)
        :continue
      end
    end
  end
end
