require_relative './base_instruction'

module Natalie
  class Compiler
    class SuperInstruction < BaseInstruction
      def initialize(args_array_on_stack:, with_block:, has_keyword_hash: false, forward_args: false)
        @args_array_on_stack = args_array_on_stack
        @with_block = with_block
        @has_keyword_hash = has_keyword_hash
        @forward_args = forward_args
      end

      def to_s
        s = 'super'
        s << ' (with_block)' if @with_block
        s << ' (args_array_on_stack)' if @args_array_on_stack
        s << ' (forward_args)' if @forward_args
        s
      end

      def generate_llvm(transform)
        if @forward_args
          array = 4
        elsif @args_array_on_stack
          array = transform.pop
        else
          argc = transform.pop
          args = transform.pop(argc)
          transform.push_call("call i64 @nat_llvm_array(i64 #{argc}, ptr #{transform.argument_array(args)})")
          array = transform.pop
        end
        receiver = transform.pop
        block = @with_block ? transform.pop : 4
        transform.push_call(
          "call i64 @nat_llvm_super(ptr %env, i64 #{receiver}, i64 #{array}, ptr #{@forward_args ? transform.function_args : 'null'}, i64 #{block}, ptr #{transform.function_block}, i32 #{@with_block ? 1 : 0}, i32 #{@has_keyword_hash ? 1 : 0})",
        )
      end
    end
  end
end
