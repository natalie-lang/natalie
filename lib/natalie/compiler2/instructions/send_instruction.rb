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

      def execute(vm)
        receiver = vm.pop
        arg_count = vm.pop
        args = []
        arg_count.times { args << vm.pop }
        result = receiver.send(@message, *args)
        vm.push result
      end
    end
  end
end
