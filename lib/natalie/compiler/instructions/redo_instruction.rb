require_relative './base_instruction'

module Natalie
  class Compiler
    class RedoInstruction < BaseInstruction
      def initialize(in_loop: false)
        @in_loop = in_loop
      end

      attr_accessor :redo_point

      def to_s
        s = 'redo'
        s << ' (in loop)'
        s
      end

      def generate_llvm(transform)
        if @in_loop
          transform.loop_jump(:redo)
        elsif @redo_point
          transform.emit "call void @nat_llvm_jump(ptr %env, i64 4, i32 1, i64 #{transform.break_point(@redo_point)})"
          transform.emit 'unreachable'
          transform.emit "#{transform.name('unreachable')}:"
          transform.push('4')
        else
          transform.emit 'call void @nat_llvm_redo()'
          transform.return_value('4')
        end
      end
    end
  end
end
