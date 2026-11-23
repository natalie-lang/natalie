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

      def generate(transform)
        if @forward_args
          args =
            'Args(args.original_size(), &tl_current_arg_stack->data()[args.original_start_index()], args.has_keyword_hash())'
        elsif @args_array_on_stack
          ary = "#{transform.pop}.as_array()"
          args = "Args(#{ary}->size(), #{ary}->data(), #{@has_keyword_hash})"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args = "Args({ #{args.join(', ')} }, #{@has_keyword_hash})"
        end

        receiver = transform.pop

        # NOTE: There is a similar line in SendInstruction#generate, but
        # this one differs in that we fall back to sending the `block`
        # from the current method, if it is set.
        current_method_block = 'block'
        block = @with_block ? "to_block(env, #{transform.pop})" : current_method_block

        transform.exec_and_push :super, "super(env, #{receiver}, #{args}, #{block})"
      end
    end
  end
end
