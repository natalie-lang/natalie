require_relative './base_instruction'

module Natalie
  class Compiler
    class YieldInstruction < BaseInstruction
      def initialize(args_array_on_stack:, has_keyword_hash:)
        @args_array_on_stack = args_array_on_stack
        @has_keyword_hash = has_keyword_hash
      end

      attr_reader :args_array_on_stack, :has_keyword_hash

      def to_s
        s = 'yield'
        s << ' (args array on stack)' if @args_array_on_stack
        s << ' (has keyword hash)' if @has_keyword_hash
        s
      end

      def generate_llvm(transform)
        if @args_array_on_stack
          transform.push_call(
            "call i64 @nat_llvm_yield_array(ptr %env, i64 #{transform.pop}, i32 #{@has_keyword_hash ? 1 : 0})",
          )
          return
        end
        argc = transform.pop
        args = transform.pop(argc)
        transform.push_call(
          "call i64 @nat_llvm_yield(ptr %env, i64 #{argc}, ptr #{transform.argument_array(args)}, i32 #{@has_keyword_hash ? 1 : 0})",
        )
      end
    end
  end
end
