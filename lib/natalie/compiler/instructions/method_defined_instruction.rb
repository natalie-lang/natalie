require_relative './base_instruction'

module Natalie
  class Compiler
    class MethodDefinedInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:, args_array_on_stack: false, has_keyword_hash: false, receiver_pushed_first: false)
        raise 'method with block is always "expression"' if with_block
        @message = message.to_sym
        @receiver_is_self = receiver_is_self
        @args_array_on_stack = args_array_on_stack
        @has_keyword_hash = has_keyword_hash
        # TODO: remove this once all call sites are sending true
        @receiver_pushed_first = receiver_pushed_first
      end

      attr_reader :message,
                  :receiver_is_self,
                  :args_array_on_stack,
                  :has_keyword_hash

      def to_s
        "method_defined #{@message.inspect}"
      end

      def generate(transform)
        unless @receiver_pushed_first
          receiver = transform.pop
        end

        if @args_array_on_stack
          transform.pop
        else
          arg_count = transform.pop
          arg_count.times { transform.pop }
        end

        if @receiver_pushed_first
          receiver = transform.pop
        end

        transform.exec(
          "if (!#{receiver}->respond_to(env, #{transform.intern(@message)})) throw new ExceptionObject"
        )
        transform.push('NilObject::the()')
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
