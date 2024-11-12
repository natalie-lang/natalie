require_relative './base_instruction'

module Natalie
  class Compiler
    class RedoInstruction < BaseInstruction
      def to_s
        'redo'
      end

      def generate(transform)
        value = transform.memoize(:obj_with_redo_flag, 'Object::_new(env, GlobalEnv::the()->Object(), {}, nullptr)');
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
