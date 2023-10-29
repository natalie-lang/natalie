require_relative './base_instruction'

module Natalie
  class Compiler
    class UndefineMethodInstruction < BaseInstruction
      def to_s
        'undefine_method'
      end

      def generate(transform)
        transform.exec_and_push(:undef_result, "self->undefine_method(env, #{transform.pop}->to_symbol(env, Object::Conversion::Strict))")
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
