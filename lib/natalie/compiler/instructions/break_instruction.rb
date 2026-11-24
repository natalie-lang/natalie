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

      def generate_llvm(transform)
        value = transform.pop
        transform.emit "call void @nat_llvm_jump(ptr %env, i64 #{value}, i32 #{@type == :return ? 1 : 0}, i64 #{transform.break_point(@break_point)})"
        transform.emit 'unreachable'
        transform.emit "#{transform.name('unreachable')}:"
        transform.push('4')
      end
    end
  end
end
