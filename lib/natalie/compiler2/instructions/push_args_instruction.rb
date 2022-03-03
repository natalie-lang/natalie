require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgsInstruction < BaseInstruction
      def initialize(for_block:, arity:)
        @for_block = for_block
        @arity = arity
      end

      def to_s
        s = 'push_args'
        s << ' (for_block)' if @for_block
        s
      end

      def generate(transform)
        args = transform.temp('args')
        if @for_block && @arity > 1
          transform.exec_and_push(:args, "argc == 1 ? to_ary(env, args[0], true) : new ArrayObject(argc, args)")
        else
          transform.exec_and_push(:args, "new ArrayObject(argc, args)")
        end
      end

      def execute(vm)
        if @for_block && @arity > 1 && vm.args.size == 1
          if vm.args.first.is_a?(Array)
            vm.push(vm.args.first)
          elsif vm.args.first.respond_to?(:to_ary)
            vm.push(vm.args.first.to_ary)
          else
            vm.push(vm.args)
          end
        else
          vm.push(vm.args)
        end
      end
    end
  end
end
