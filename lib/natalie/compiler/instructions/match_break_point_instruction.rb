require_relative './base_instruction'

module Natalie
  class Compiler
    class MatchBreakPointInstruction < BaseInstruction
      def initialize(break_point)
        @break_point = break_point
      end

      attr_reader :break_point

      def to_s
        "match_break_point #{@break_point}"
      end

      def generate(transform)
        value = transform.temp('break_value')
        code = [
          "Value #{value} = Value::nil()",
          "if (exception->is_local_jump_error_with_break_point(#{@break_point})) {",
          "#{value} = exception->ivar_get(env, \"@exit_value\"_s)",
          '} else {',
          'throw exception',
          '}',
        ]
        transform.exec(code)
        transform.push(value)
      end

      def execute(vm)
        exception = vm.global_variables[:$!]
        if exception.instance_variable_get(:@break_point) == @break_point
          value = exception.instance_variable_get(:@exit_value)
          vm.push(value)
        else
          raise exception
        end
      end
    end
  end
end
