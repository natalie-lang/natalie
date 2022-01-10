require_relative './base_instruction'

module Natalie
  class Compiler2
    class SendInstruction < BaseInstruction
      def initialize(message)
        @message = message
      end

      def to_s
        "send #{@message}"
      end

      def to_cpp(transform)
        receiver = transform.pop
        arg_count = transform.pop
        args = []
        arg_count.times { args << transform.pop }
        if args.any?
          "#{receiver}->send(env, #{@message.to_s.inspect}_s, { #{args.join(', ')} })"
        else
          "#{receiver}->send(env, #{@message.to_s.inspect}_s)"
        end
      end
    end
  end
end
