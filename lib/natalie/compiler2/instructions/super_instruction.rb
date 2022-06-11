require_relative './base_instruction'

module Natalie
  class Compiler2
    class SuperInstruction < BaseInstruction
      def initialize(args_array:)
        @args_array = args_array
      end

      def to_s
        s = 'super'
        s << ' (args array)' if @args_array
        s
      end

      def generate(transform)
        receiver = transform.pop
        if @args_array
          args = transform.pop
          arg_count = "#{args}->size()"
          args_array = "#{args}->data()"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_array = transform.temp('args')
          transform.exec "Value #{args_array}[] = { #{args.join(', ')} };"
        end
        block = 'nullptr' # TODO: ability to pass block with super
        transform.exec_and_push :super, "super(env, #{receiver}, #{arg_count}, #{args_array}, #{block})"
      end

      def execute(vm)
        receiver = vm.pop
        args = if @args_array
                 vm.pop
               else
                 arg_count = vm.pop
                 args = []
                 arg_count.times { args.unshift vm.pop }
                 args
               end
        vm.with_self(receiver) do
          # TODO: ability to pass block
          result = receiver.method(vm.method_name).super_method.call(*args)
          vm.push result
        end
      end
    end
  end
end
