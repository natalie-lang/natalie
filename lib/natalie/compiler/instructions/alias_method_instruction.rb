require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(new_name)
    # push(old_name)
    # alias
    class AliasMethodInstruction < BaseInstruction
      def to_s
        'alias_method'
      end

      def generate_llvm(transform)
        old_name = transform.pop
        new_name = transform.pop
        transform.emit "call void @nat_llvm_alias(ptr %env, i64 %self, i64 #{new_name}, i64 #{old_name})"
      end

      def generate(transform)
        old_name = transform.pop
        new_name = transform.pop
        transform.exec("Object::method_alias(env, self, #{new_name}, #{old_name})")
      end
    end
  end
end
