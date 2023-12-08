require_relative './base_instruction'

module Natalie
  class Compiler
    class PatternEqualCheckInstruction < BaseInstruction
      def to_s
        'pattern_equal_check'
      end

      def generate(transform)
        desired = transform.pop
        actual = transform.pop
        transform.exec(
          "if (#{desired}->send(env, \"===\"_s, { #{actual} })->is_falsey())" \
          "env->raise(\"NoMatchingPatternError\", \"{} === {} does not return true\", #{desired}->inspect_str(env), #{actual}->inspect_str(env))"
        )
      end

      def execute(_vm)
        raise 'todo'
      end
    end
  end
end
