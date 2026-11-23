require_relative './base_instruction'

module Natalie
  class Compiler
    class HashMergeInstruction < BaseInstruction
      def to_s
        'hash_merge'
      end

      def generate_llvm(transform)
        other = transform.pop
        transform.emit "call void @nat_llvm_hash_merge(ptr %env, i64 #{transform.peek}, i64 #{other})"
      end

      def generate(transform)
        hash2 = transform.pop
        return if hash2 == 'Value::nil()'

        hash = transform.peek
        transform.exec("if (!#{hash2}.is_nil()) {")
        transform.exec("  #{hash}.as_hash()->merge_in_place(env, Args({ #{hash2} }), nullptr)")
        transform.exec('}')
      end
    end
  end
end
