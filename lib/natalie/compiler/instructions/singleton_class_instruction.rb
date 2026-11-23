require_relative './base_instruction'

module Natalie
  class Compiler
    class SingletonClassInstruction < BaseInstruction
      def to_s
        'singleton_class'
      end

      def generate(transform)
        obj = transform.pop
        transform.push("Object::singleton_class(env, #{obj})")
      end
    end
  end
end
