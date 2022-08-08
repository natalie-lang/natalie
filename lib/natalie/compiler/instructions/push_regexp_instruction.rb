require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      def initialize(regexp)
        @regexp = regexp
      end

      def to_s
        "push_regexp #{@regexp.inspect}"
      end

      def generate(transform)
        transform.exec_and_push('regexp', "Value(RegexpObject::literal(env, #{@regexp.source.inspect}, #{@regexp.options}))")
      end

      def execute(vm)
        vm.push(@regexp.dup)
      end
    end
  end
end
