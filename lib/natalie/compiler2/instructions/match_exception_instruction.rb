require_relative './base_instruction'

module Natalie
  class Compiler2
    class MatchExceptionInstruction < BaseInstruction
      def to_s
        'match_exception'
      end

      def generate(transform)
        ary = transform.pop
        result = transform.temp('match_exception_result')
        code = "exception->match_rescue_array(env, #{ary})"
        transform.exec_and_push(result, code)
      end

      def execute(vm)
        ary = vm.pop
        vm.push(ary.any? { |e| e === vm.global_variables[:$!] })
      end
    end
  end
end
