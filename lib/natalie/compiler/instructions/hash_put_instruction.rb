require_relative './base_instruction'

module Natalie
  class Compiler
    class HashPutInstruction < BaseInstruction
      def to_s
        'hash_put'
      end

      def generate_llvm(transform)
        value, key = transform.pop, transform.pop
        transform.emit "call void @nat_llvm_hash_put(ptr %env, i64 #{transform.peek}, i64 #{key}, i64 #{value})"
      end

      def generate(transform)
        value = transform.pop
        key = transform.pop
        hash = transform.peek
        transform.exec("#{hash}.as_hash()->put(env, #{key}, #{value})")
      end
    end
  end
end
