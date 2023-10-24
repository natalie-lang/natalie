require_relative './base_instruction'

module Natalie
  class Compiler
    class SuperInstruction < BaseInstruction
      def initialize(args_array_on_stack:, with_block:, has_keyword_hash: false)
        @args_array_on_stack = args_array_on_stack
        @with_block = with_block
        @has_keyword_hash = has_keyword_hash
      end

      def to_s
        s = 'super'
        s << ' with block' if @with_block
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate(transform)
        if @args_array_on_stack
          args = transform.pop
          arg_count = "#{args}->as_array()->size()"
          args_array_on_stack = "#{args}->as_array()->data()"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_array_on_stack = transform.temp('args')
          transform.exec "Value #{args_array_on_stack}[] = { #{args.join(', ')} };"
        end

        receiver = transform.pop

        # NOTE: There is a similar line in SendInstruction#generate, but
        # this one differs in that we fall back to sending the `block`
        # from the current method, if it is set.
        current_method_block = 'block'
        block = @with_block ? "to_block(env, #{transform.pop})" : current_method_block

        transform.exec_and_push :super, "super(env, #{receiver}, Args(#{arg_count}, #{args_array_on_stack}, #{@has_keyword_hash ? 'true' : 'false'}), #{block})"
      end

      def execute(vm)
        args = if @args_array_on_stack
                 vm.pop
               else
                 arg_count = vm.pop
                 args = []
                 arg_count.times { args.unshift vm.pop }
                 args
               end
        receiver = vm.pop
        block = @with_block ? vm.pop : vm.block
        vm.with_self(receiver) do
          result = receiver.method(vm.method_name).super_method.call(*args, &block)
          vm.push result
        end
      end
    end
  end
end
