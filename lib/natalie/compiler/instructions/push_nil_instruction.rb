require_relative './base_instruction'

module Natalie
  class Compiler
    class PushNilInstruction < BaseInstruction
      def to_s
        'push_nil'
      end

      def generate(transform)
        transform.push_nil
      end

      def execute(vm)
        vm.push(nil)
      end

      def serialize
        [instruction_number].pack('C')
      end

      def self.deserialize(_)
        new
      end
    end
  end
end
