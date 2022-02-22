require_relative './base_instruction'

module Natalie
  class Compiler2
    class SendInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:)
        @message = message.to_sym
        @receiver_is_self = receiver_is_self
        @with_block = with_block
      end

      attr_accessor :break_point

      def to_s
        s = "send #{@message.inspect}"
        s << ' to self' if @receiver_is_self
        s << ' with block' if @with_block
        s << " with break_point #{@break_point}" if @break_point
        s
      end

      def generate(transform)
        receiver = transform.pop
        arg_count = transform.pop
        args = []
        arg_count.times { args.unshift transform.pop }
        block = @with_block ? "to_block(env, #{transform.pop})" : 'nullptr'
        send_call = "#{receiver}.#{method}(env, #{@message.to_s.inspect}_s, { #{args.join(', ')} }, #{block})"
        if @break_point
          result = transform.temp("send_#{@message}")
          transform.exec([
            "Value #{result}",
            'try {',
            "  #{result} = #{send_call}",
            '} catch (ExceptionObject *exception) {',
            "  if (!exception->is_local_jump_error_with_break_point(env, #{@break_point})) throw exception",
            "  #{result} = exception->ivar_get(env, \"@exit_value\"_s)",
            '}'
          ])
          transform.push(result)
        else
          transform.exec_and_push("send_#{@message}", send_call)
        end
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
        call = -> do
          if @with_block
            block = vm.pop
            receiver.send(method, @message, *args, &block)
          else
            receiver.send(method, @message, *args)
          end
        end

        if @break_point
          begin
            result = call.()
          rescue LocalJumpError => e
            raise unless e.instance_variable_get(:@break_point) == @break_point
            result = e.exit_value
          end
        else
          result = call.()
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
