require_relative './base_instruction'

module Natalie
  class Compiler
    class ToHashInstruction < BaseInstruction
      def to_s
        'to_hash'
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:array, "to_hash(env, #{obj})")
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
