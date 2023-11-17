require_relative './base_instruction'

module Natalie
  class Compiler
    # This instruction is really just used for breaking from a `while` loop.
    class BreakOutInstruction < BaseInstruction
      def initialize(while_instruction:)
        @while_instruction = while_instruction
      end

      def to_s
        'break_out'
      end

      attr_reader :while_instruction

      def generate(transform)
        value = transform.pop
        transform.exec("#{while_instruction.result_name} = #{value}")
        transform.exec("break")
      end

      def execute(vm)
        :break_out
      end
    end
  end
end
