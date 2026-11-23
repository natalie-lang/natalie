require_relative './base_instruction'

module Natalie
  class Compiler
    class RedoInstruction < BaseInstruction
      def initialize(in_loop: false)
        @in_loop = in_loop
      end

      def to_s
        s = 'redo'
        s << ' (in loop)'
        s
      end

      def generate_llvm(transform)
        if @in_loop
          transform.loop_jump(:redo)
        else
          transform.emit 'call void @nat_llvm_redo()'
          transform.return_value('4')
        end
      end

      def generate(transform)
        transform.exec('FiberObject::current()->set_redo_flag()')
        if @in_loop
          transform.exec('continue')
        else
          transform.exec('return Value::nil()')
        end
        transform.push_nil
      end
    end
  end
end
