require_relative './base_instruction'

module Natalie
  class Compiler
    class BreakInstruction < BaseInstruction
      def to_s
        s = 'break'
        s << " #{@break_point}" if @break_point
        s
      end

      attr_accessor :break_point

      def generate(transform)
        raise 'no break point set' unless @break_point

        value = transform.pop
        transform.exec("env->raise_local_jump_error(#{value}, LocalJumpErrorType::Break, #{@break_point})")
        transform.push('Value(NilObject::the())')
      end

      def execute(vm)
        raise 'no break point set' unless @break_point

        error = LocalJumpError.new('break from proc-closure')
        error.instance_variable_set(:@break_point, @break_point)
        error.instance_variable_set(:@exit_value, vm.pop)
        raise error
      end
    end
  end
end
