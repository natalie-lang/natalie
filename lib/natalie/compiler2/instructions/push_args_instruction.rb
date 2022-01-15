require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgsInstruction < BaseInstruction
      def to_s
        'push_args'
      end

      def generate(transform)
        args = transform.temp('args')
        transform.exec("auto #{args} = new ArrayObject(argc, args)")
        transform.push(args)
      end

      def execute(vm)
        vm.push(vm.args)
      end
    end
  end
end
