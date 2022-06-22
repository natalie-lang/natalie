require_relative './base_instruction'

module Natalie
  class Compiler2
    class YieldInstruction < BaseInstruction
      def to_s
        'yield'
      end

      def generate(transform)
        arg_count = transform.pop
        args = []
        arg_count.times { args.unshift transform.pop }
        args_array = transform.temp('args')
        transform.exec "Value #{args_array}[] = { #{args.join(', ')} };"
        transform.exec_and_push :yield, "NAT_RUN_BLOCK_FROM_ENV(env, Args(#{arg_count}, #{args_array}))"
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
