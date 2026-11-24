require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(block) if block
    # push(receiver)
    # push(*args)
    # push(argc)
    # send(message)
    class SendInstruction < BaseInstruction
      def initialize(
        message,
        receiver_is_self:,
        with_block:,
        file:,
        line:,
        args_array_on_stack: false,
        has_keyword_hash: false,
        forward_args: false
      )
        # the message to send
        @message = message.to_sym

        # implied self, no explicit receiver, e.g. foo vs something.foo
        @receiver_is_self = receiver_is_self

        # the args array is already on the stack and ready;
        # no need to collect individual args
        @args_array_on_stack = args_array_on_stack

        # a block is on the stack
        @with_block = with_block

        # a keyword hash is last in the args
        @has_keyword_hash = has_keyword_hash

        # true when special ... syntax was used
        @forward_args = forward_args

        # source location info
        @file = file
        @line = line
      end

      attr_reader :message,
                  :receiver_is_self,
                  :with_block,
                  :args_array_on_stack,
                  :has_keyword_hash,
                  :forward_args,
                  :file,
                  :line

      def with_block?
        !!@with_block
      end

      def to_s
        s = "send #{@message.inspect}"
        s << ' to self' if @receiver_is_self
        s << ' with block' if @with_block
        s << ' (args_array_on_stack)' if @args_array_on_stack
        s << ' (has_keyword_hash)' if @has_keyword_hash
        s << ' (forward_args)' if @forward_args
        s
      end

      def generate_llvm(transform)
        if @forward_args || @args_array_on_stack
          array = @forward_args ? 4 : transform.pop
          receiver = transform.pop
          block = @with_block ? transform.pop : 4
          setter = @message.to_s.match?(/\w=$|\[\]=$/)
          transform.push_call(
            "call i64 @nat_llvm_send_dynamic(ptr %env, i64 %self, i64 #{receiver}, ptr #{transform.string(@message.to_s)}, i64 #{array}, ptr #{@forward_args ? '%args' : 'null'}, i32 #{@receiver_is_self ? 0 : 1}, i32 #{@has_keyword_hash ? 1 : 0}, ptr #{transform.string(@file || transform.source_path)}, i64 #{@line || 1}, i64 #{block}, i32 #{setter ? 1 : 0})",
          )
          return
        end
        argc = transform.pop
        raise CompileError, 'LLVM: expected static argument count' unless argc.is_a?(Integer)
        args = transform.pop(argc)
        receiver = transform.pop
        block = @with_block ? transform.pop : 4
        call =
          "call i64 @nat_llvm_send(ptr %env, i64 %self, i64 #{receiver}, ptr #{transform.string(@message.to_s)}, i64 #{argc}, ptr #{transform.argument_array(args)}, i32 #{@receiver_is_self ? 0 : 1}, i32 #{@has_keyword_hash ? 1 : 0}, ptr #{transform.string(@file || transform.source_path)}, i64 #{@line || 1}, i64 #{block})"
        transform.push_call(call)
        if @message.to_s.match?(/\w=$|\[\]=$/)
          transform.pop
          transform.push(args.last)
        end
      end

      def to_method_defined_instruction
        MethodDefinedInstruction.new(
          message,
          receiver_is_self: @receiver_is_self,
          with_block: @with_block,
          args_array_on_stack: @args_array_on_stack,
          has_keyword_hash: @has_keyword_hash,
        )
      end

      private

      def method
        if @receiver_is_self
          :send
        else
          :public_send
        end
      end
    end
  end
end
