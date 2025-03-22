require_relative './base_instruction'

module Natalie
  class Compiler
    class UndefineMethodInstruction < BaseInstruction
      def to_s
        'undefine_method'
      end

      def generate(transform)
        transform.exec_and_push(
          :undef_result,
          "Object::undefine_method(env, self, #{transform.pop}.to_symbol(env, Value::Conversion::Strict))",
        )
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
