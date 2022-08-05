require_relative './base_instruction'

module Natalie
  class Compiler2
    class YieldInstruction < BaseInstruction
      def initialize(args_array_on_stack:)
        @args_array_on_stack = args_array_on_stack
        # TODO: @has_keyword_hash
      end

      def to_s
        s = 'yield'
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate(transform)
        if @args_array_on_stack
          args = "#{transform.pop}->as_array()"
          args_list = "Args(#{args}, #{@has_keyword_hash ? 'true' : 'false'})"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_list = "Args({ #{args.join(', ')} }, #{@has_keyword_hash ? 'true' : 'false'})"
        end
        transform.exec_and_push :yield, "NAT_RUN_BLOCK_FROM_ENV(env, #{args_list})"
      end

      def execute(vm)
        arg_count = vm.pop
        args = []
        arg_count.times { args.unshift vm.pop }
        raise LocalJumpError.new('no block given') unless vm.block
        result = vm.block.call(*args)
        vm.push result
      end
    end
  end
end
