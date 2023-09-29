require_relative './base_instruction'

module Natalie
  class Compiler
    class RedoInstruction < BaseInstruction
      def to_s
        'redo'
      end

      def generate(transform)
        value = transform.memoize(:nil_with_redo_flag, 'Value(NilObject::the())')
        transform.exec("#{value}->add_redo_flag()")
        transform.exec("return #{value}")
        transform.push_nil
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
