require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushNilInstruction < BaseInstruction
      def to_s
        'push_nil'
      end

      def to_cpp(_)
        'NilObject::the()'
      end

      def execute(vm)
        vm.push(nil)
      end
    end
  end
end
