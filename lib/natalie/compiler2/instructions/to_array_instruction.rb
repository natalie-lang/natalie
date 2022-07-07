require_relative './base_instruction'

module Natalie
  class Compiler2
    class ToArrayInstruction < BaseInstruction
      def to_s
        'to_array'
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:array, "to_ary_for_masgn(env, #{obj})")
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
