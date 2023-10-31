require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(new_name)
    # push(old_name)
    # alias
    class AliasGlobalInstruction < BaseInstruction
      def to_s
        'alias_global'
      end

      def generate(transform)
        old_name = transform.pop
        new_name = transform.pop
        transform.exec("env->global_alias(#{new_name}->to_symbol(env, Object::Conversion::Strict), #{old_name}->to_symbol(env, Object::Conversion::Strict))")
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
