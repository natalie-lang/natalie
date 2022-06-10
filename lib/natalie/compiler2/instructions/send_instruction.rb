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
      def initialize(message, receiver_is_self:, with_block:, file:, line:)
        @message = message.to_sym
        @receiver_is_self = receiver_is_self
        @with_block = with_block
        @file = file
        @line = line
      end

      attr_reader :file, :line

      def with_block?
        !!@with_block
      end

      def to_s
        s = "send #{@message.inspect}"
        s << ' to self' if @receiver_is_self
        s << ' with block' if @with_block
        s
      end

      def generate(transform)
        receiver = transform.pop

        if (arg_count = transform.pop) == -1
          args = "#{transform.pop}->as_array()"
          args_array = "#{args}->size(), #{args}->data()"
        else
          args = []
          arg_count.times { args.unshift transform.pop }
          args_array = "{ #{args.join(', ')} }"
        end

        transform.exec("env->set_file(#{@file.inspect})") if @file
        transform.exec("env->set_line(#{@line.inspect})") if @line

        block = @with_block ? "to_block(env, #{transform.pop})" : 'nullptr'
        transform.exec_and_push(
          "send_#{@message}",
          "#{receiver}.#{method}(env, #{@message.to_s.inspect}_s, #{args_array}, #{block})"
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

        if arg_count == -1
          args = vm.pop
        else
          args = []
          arg_count.times { args.unshift vm.pop }
        end

        vm.with_self(receiver) do
          result =
            if @with_block
              block = vm.pop
              receiver.send(method, @message, *args, &block)
            else
              receiver.send(method, @message, *args)
            end
          vm.push result
        end
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
