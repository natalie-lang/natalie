require_relative './base_instruction'

module Natalie
  class Compiler2
    class StringAppendInstruction < BaseInstruction
      def to_s
        'string_append'
      end

      def generate(transform)
        new_part = transform.pop
        target = transform.peek
        transform.exec("#{target}->as_string()->append(env, #{new_part});")
      end

      def execute(vm)
        new_part = vm.pop
        target = vm.peek
        target.concat(new_part.to_s)
      end
    end
  end
end
