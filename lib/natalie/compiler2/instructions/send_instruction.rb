require_relative './base_instruction'

module Natalie
  class Compiler2
    # use:
    # push(block) if block
    # push(*args)
    # push(argc)
    # push(receiver)
    # send(message)
    class SendInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:)
        @message = message.to_sym
        @receiver_is_self = receiver_is_self
        @with_block = with_block
      end

      def to_s
        s = "send #{@message.inspect}"
        s << ' to self' if @receiver_is_self
        s << ' with block' if @with_block
        s
      end

      def generate(transform)
        receiver = transform.pop
        arg_count = transform.pop
        args = []
        arg_count.times { args.unshift transform.pop }
        block = @with_block ? "to_block(env, #{transform.pop})" : 'nullptr'
        transform.exec_and_push(
          "send_#{@message}",
          "#{receiver}.#{method}(env, #{@message.to_s.inspect}_s, { #{args.join(', ')} }, #{block})"
        )
      end

      def execute(vm)
        receiver = vm.pop
        arg_count = vm.pop
        if arg_count.zero? && %i[public private protected].include?(@message)
          vm.method_visibility = @message
          vm.push(receiver)
          return
        end

        args = []
        arg_count.times { args.unshift vm.pop }
        self_was = vm.self
        vm.self = receiver
        result =
          if @with_block
            block = vm.pop
            receiver.send(method, @message, *args, &block)
          else
            receiver.send(method, @message, *args)
          end
        vm.push result
        vm.self = self_was
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
