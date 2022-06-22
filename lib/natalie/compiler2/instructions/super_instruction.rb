require_relative './base_instruction'

module Natalie
  class Compiler2
    class SuperInstruction < BaseInstruction
      def initialize(args_array_on_stack:, with_block_pass:)
        @args_array_on_stack = args_array_on_stack
        @with_block_pass = with_block_pass
      end

      def to_s
        s = 'super'
        s << ' with block pass' if @with_block_pass
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate(transform)
        receiver = transform.pop
        if @args_array_on_stack
          args = transform.pop
          arg_count = "#{args}->size()"
          args_array_on_stack = "#{args}->data()"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_array_on_stack = transform.temp('args')
          transform.exec "Value #{args_array_on_stack}[] = { #{args.join(', ')} };"
        end
        block = @with_block_pass ? "to_block(env, #{transform.pop})" : 'block'
        transform.exec_and_push :super, "super(env, #{receiver}, Args(#{arg_count}, #{args_array_on_stack}), #{block})"
      end

      def execute(vm)
        receiver = vm.pop
        args = if @args_array_on_stack
                 vm.pop
               else
                 arg_count = vm.pop
                 args = []
                 arg_count.times { args.unshift vm.pop }
                 args
               end
        block = @with_block_pass ? vm.pop : vm.block
        vm.with_self(receiver) do
          result = receiver.method(vm.method_name).super_method.call(*args, &block)
          vm.push result
        end
      end
    end
  end
end
