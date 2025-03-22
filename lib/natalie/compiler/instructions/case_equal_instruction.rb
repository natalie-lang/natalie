require_relative './base_instruction'

module Natalie
  class Compiler
    class CaseEqualInstruction < BaseInstruction
      def initialize(is_splat:)
        @is_splat = is_splat
      end

      attr_reader :is_splat

      def to_s
        s = 'case_equal'
        s << ' (is splat)' if @is_splat
        s
      end

      def generate(transform)
        when_value = transform.pop
        case_value = transform.peek
        transform.exec_and_push(
          :is_case_equal,
          "is_case_equal(env, #{case_value}, #{when_value}, #{@is_splat ? 'true' : 'false'})",
        )
      end

      def execute(vm)
        when_value = vm.pop
        case_value = vm.peek
        vm.push(when_value === case_value)
      end
    end
  end
end
