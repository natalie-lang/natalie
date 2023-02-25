require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(block) if block
    # push(*args)
    # push(argc)
    # push(receiver)
    # send(message)
    class SendInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:, file:, line:, args_array_on_stack: false, has_keyword_hash: false, forward_args: false)
        # the message to send
        @message = message.to_sym

        # implied self, no explicit receiver, e.g. foo vs something.foo
        @receiver_is_self = receiver_is_self

        # the args array is already on the stack and ready;
        # no need to collect individual args
        @args_array_on_stack = args_array_on_stack

        # a block is on the stack
        @with_block = with_block

        # a bare hash (probably a keyword hash) is last in the args
        @has_keyword_hash = has_keyword_hash

        # true when special ... syntax was used
        @forward_args = forward_args

        # source location info
        @file = file
        @line = line
      end

      attr_reader :message,
                  :receiver_is_self,
                  :with_block,
                  :args_array_on_stack,
                  :has_keyword_hash,
                  :file,
                  :line

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
        if @args_array_on_stack
          if @forward_args
            args_list = transform.pop
          else
            args = "#{transform.pop}->as_array()"
            args_list = "Args(#{args}, #{@has_keyword_hash ? 'true' : 'false'})"
          end
        else
          arg_count = transform.pop
          raise "bad argc #{arg_count.inspect} for SendInstruction #{@message.inspect}" unless arg_count.is_a?(Integer)
          args = []
          arg_count.times { args.unshift transform.pop }
          args_list = "Args({ #{args.join(', ')} }, #{@has_keyword_hash ? 'true' : 'false'})"
        end

        transform.set_file(@file)
        transform.set_line(@line)

        receiver = transform.pop
        unless receiver.is_a?(String)
          puts "Something went wrong: bad receiver #{receiver.inspect} for SendInstruction #{@message.inspect}"
          puts "Got arg_count = #{arg_count.inspect}"
          puts "Got args = #{args.inspect}"
          exit 1
        end

        block = @with_block ? "to_block(env, #{transform.pop})" : 'nullptr'
        transform.exec_and_push(
          "send_#{@message}",
          "#{receiver}.#{method}(env, #{transform.intern(@message)}, #{args_list}, #{block}, self)"
        )
      end

      def execute(vm)
        if @args_array_on_stack
          args = vm.pop
        else
          arg_count = vm.pop
          args = []
          arg_count.times { args.unshift vm.pop }
        end

        receiver = vm.pop

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

      def to_method_defined_instruction
        MethodDefinedInstruction.new(
          message,
          receiver_is_self: @receiver_is_self,
          with_block: @with_block,
          args_array_on_stack: @args_array_on_stack,
          has_keyword_hash: @has_keyword_hash,
        )
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
