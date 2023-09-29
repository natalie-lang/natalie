require_relative './base_instruction'

module Natalie
  class Compiler
    class BreakInstruction < BaseInstruction
      def initialize(type: :break)
        super()
        raise 'bad type' unless %i[break return].include?(type)
        @type = type
      end

      def to_s
        s = 'break'
        s << " #{@break_point}" if @break_point
        s << ' (return)' if @type == :return
        s
      end

      attr_accessor :break_point

      def generate(transform)
        raise 'no break point set' unless @break_point

        value = transform.pop
        type = @type.to_s.capitalize
        transform.exec("env->raise_local_jump_error(#{value}, LocalJumpErrorType::#{type}, #{@break_point})")
        transform.push_nil
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
