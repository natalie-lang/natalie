require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstSetInstruction < BaseInstruction
      def initialize(name, strict:)
        @name = name.to_sym
        @strict = strict
      end

      attr_reader :name

      def to_s
        s = "const_set #{@name}"
        s << ' (strict)' if @strict
        s
      end

      def generate_llvm(transform)
        namespace, value = transform.pop, transform.pop
        transform.emit "call void @nat_llvm_const_set(ptr %env, i64 #{namespace}, ptr #{transform.string(@name.to_s)}, i64 #{value}, i32 #{@strict ? 1 : 0})"
      end
    end
  end
end
