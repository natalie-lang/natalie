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
      def initialize(message, receiver_is_self:, with_block:, file:, line:, args_array_on_stack: false)
        # the message to send
        @message = message.to_sym

        # implied self, no explicit receiver, e.g. foo vs something.foo
        @receiver_is_self = receiver_is_self

        # the args array is already on the stack and ready;
        # no need to collect individual args
        @args_array_on_stack = args_array_on_stack

        # a block is on the stack
        @with_block = with_block

        # source location info
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
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate(transform)
        receiver = transform.pop

        if @args_array_on_stack
          args = "#{transform.pop}->as_array()"
          args_array = "Args(#{args})"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_array = "std::initializer_list<Value> { #{args.join(', ')} }"
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

        if @args_array_on_stack
          args = vm.pop
        else
          arg_count = vm.pop
          args = []
          arg_count.times { args.unshift vm.pop }
        end

        if args.empty? && %i[public private protected].include?(@message)
          vm.method_visibility = @message
          vm.push(receiver)
          return
        end

        vm.with_self(receiver) do
          block = vm.pop if @with_block
          result = receiver.send(method, @message, *args, &block)
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
