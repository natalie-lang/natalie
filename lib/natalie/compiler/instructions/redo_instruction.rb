require_relative './base_instruction'

module Natalie
  class Compiler
    class RedoInstruction < BaseInstruction
      def to_s
        'redo'
      end

      def generate(transform)
        transform.exec('FiberObject::current()->set_redo_block()')
        transform.exec('return Value::nil()')
        transform.push_nil
      end

      def execute(_vm)
        raise 'todo'
      end
    end
  end
end
