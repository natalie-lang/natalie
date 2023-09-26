require_relative './base_instruction'

module Natalie
  class Compiler
    class ShellInstruction < BaseInstruction
      def to_s
        'shell'
      end

      def generate(transform)
        command = transform.pop
        transform.exec_and_push(:shell, "shell_backticks(env, #{command})")
      end

      def execute(vm)
        command = vm.pop
        vm.push(`#{command}`)
      end
    end
  end
end
