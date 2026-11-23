require_relative './base_instruction'

module Natalie
  class Compiler
    class MethodDefinedInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:, args_array_on_stack: false, has_keyword_hash: false)
        raise 'method with block is always "expression"' if with_block
        @message = message.to_sym
        @receiver_is_self = receiver_is_self
        @args_array_on_stack = args_array_on_stack
        @has_keyword_hash = has_keyword_hash
      end

      attr_reader :message, :receiver_is_self, :args_array_on_stack, :has_keyword_hash

      def to_s
        "method_defined #{@message.inspect}"
      end

      def generate_llvm(transform)
        if @args_array_on_stack
          transform.pop
        else
          transform.pop(transform.pop)
        end
        receiver = transform.pop
        transform.emit "call void @nat_llvm_method_defined(ptr %env, i64 #{receiver}, ptr #{transform.string(@message.to_s)}, i32 #{receiver == '%self' ? 1 : 0})"
        transform.push('4')
      end

      def generate(transform)
        if @args_array_on_stack
          transform.pop
        else
          arg_count = transform.pop
          arg_count.times { transform.pop }
        end

        receiver = transform.pop

        # NOTE: This seems hacky but defined?(puts) returns true even though it's a private method so
        # we have to include all methods in this case
        include_all = receiver == 'self'
        transform.exec(
          "if (!#{receiver}.respond_to(env, #{transform.intern(@message)}, #{include_all})) throw ExceptionObject::create()",
        )
        transform.push_nil
      end
    end
  end
end
