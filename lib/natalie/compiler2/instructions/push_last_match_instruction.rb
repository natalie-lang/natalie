require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushLastMatchInstruction < BaseInstruction
      def initialize(to_s:)
        @to_s = to_s
      end

      def to_s
        s = 'last_match'
        s << ' (to_s)' if @to_s
        s
      end

      def generate(transform)
        transform.push("Value(env->last_match(#{@to_s ? 'true' : 'false'}))")
      end

      def execute(vm)
        match = $~
        match = match&.to_s if @to_s
        vm.push(match)
      end
    end
  end
end
