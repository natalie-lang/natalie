require_relative './base_instruction'

module Natalie
  class Compiler
    class MatchExceptionInstruction < BaseInstruction
      def to_s
        'match_exception'
      end

      def generate(transform)
        ary = transform.pop
        code = "exception->match_rescue_array(env, #{ary})"
        transform.exec_and_push(:match_exception_result, code)
      end
    end
  end
end
