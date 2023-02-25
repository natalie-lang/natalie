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

      attr_reader :message,
                  :receiver_is_self,
                  :args_array_on_stack,
                  :has_keyword_hash

      def to_s
        "method_defined #{@message.inspect}"
      end

      def generate(transform)
        if @args_array_on_stack
          transform.pop
        else
          arg_count = transform.pop
          arg_count.times { transform.pop }
        end

        receiver = transform.pop

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
