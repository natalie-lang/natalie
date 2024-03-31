require_relative './base_instruction'

module Natalie
  class Compiler
    class StringAppendInstruction < BaseInstruction
      def to_s
        'string_append'
      end

      def generate(transform)
        new_part = transform.pop
        target = transform.peek
        transform.exec("#{target}->as_string()->append(#{new_part}.send(env, \"to_s\"_s));")
      end

      def execute(vm)
        new_part = vm.pop
        target = vm.peek
        target.concat(new_part.to_s)
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
