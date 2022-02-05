require_relative './base_instruction'

module Natalie
  class Compiler2
    class SendInstruction < BaseInstruction
      def initialize(message, with_block:)
        @message = message.to_sym
        @with_block = with_block
      end

      def to_s
        s = "send #{@message}"
        s << ' with block' if @with_block
        s
      end

      def generate(transform)
        receiver = transform.pop
        arg_count = transform.pop
        args = []
        arg_count.times { args.unshift transform.pop }
        block = @with_block ? transform.pop : 'nullptr'
        result = transform.temp("send_#{@message}")
        transform.exec(
          # FIXME: use public_send unless receiver == :self
          "auto #{result} = #{receiver}.send(env, #{@message.to_s.inspect}_s, { #{args.join(', ')} }, #{block})",
        )
        transform.push(result)
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
            # FIXME: use public_send unless receiver == :self
            receiver.send(@message, *args, &block)
          else
            # FIXME: use public_send unless receiver == :self
            receiver.send(@message, *args)
          end
        vm.push result
        vm.self = self_was
      end
    end
  end
end
