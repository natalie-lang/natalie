require_relative './base_instruction'

module Natalie
  class Compiler
    class HashDeleteInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "hash_delete #{@name}"
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_hash_delete(ptr %env, i64 #{transform.peek}, ptr #{transform.string(@name.to_s)})",
        )
      end

      def generate(transform)
        hash = transform.peek
        transform.exec_and_push(@name, "#{hash}.as_hash()->remove(env, #{transform.intern(@name)}).value()")
      end
    end
  end
end
