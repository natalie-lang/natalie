require_relative './base_instruction'

module Natalie
  class Compiler2
    class SendInstruction < BaseInstruction
      def initialize(message, with_block:)
        @message = message
        @with_block = with_block
      end

      def to_s
        s = "send #{@message}"
        s << ' with block' if @with_block
        s
      end

      def to_cpp(transform)
        receiver = transform.pop
        arg_count = transform.pop
        args = []
        arg_count.times { args << transform.pop }
        block = @with_block ? transform.pop : 'nullptr'
        "#{receiver}->send(env, #{@message.to_s.inspect}_s, { #{args.join(', ')} }, #{block})"
      end

      def execute(vm)
        receiver = vm.pop
        arg_count = vm.pop
        args = []
        arg_count.times { args.unshift vm.pop }
        self_was = vm.self
        vm.self = receiver
        result = if @with_block
          block = vm.pop
          receiver.send(@message, *args, &block)
        else
          receiver.send(@message, *args)
        end
        vm.push result
        vm.self = self_was
      end
    end
  end
end
