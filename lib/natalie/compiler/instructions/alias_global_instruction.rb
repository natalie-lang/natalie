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

      def generate_llvm(transform)
        old_name, new_name = transform.pop, transform.pop
        transform.push_call("call i64 @nat_llvm_alias_global(ptr %env, i64 #{new_name}, i64 #{old_name})")
      end
    end
  end
end
