require_relative './base_instruction'

module Natalie
  class Compiler2
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
        receiver = transform.pop

        if @args_array_on_stack
          transform.pop
        else
          arg_count = transform.pop
          arg_count.times { transform.pop }
        end

        transform.exec(
          "if (!#{receiver}->respond_to(env, #{@message.to_s.inspect}_s)) throw new ExceptionObject"
        )
        transform.push('NilObject::the()')
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
