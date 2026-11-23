require_relative './base_instruction'

module Natalie
  class Compiler
    class SingletonClassInstruction < BaseInstruction
      def to_s
        'singleton_class'
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_singleton_class(ptr %env, i64 #{transform.pop})")
      end

      def generate(transform)
        obj = transform.pop
        transform.push("Object::singleton_class(env, #{obj})")
      end
    end
  end
end
