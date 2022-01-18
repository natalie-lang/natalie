require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushBoolInstruction < BaseInstruction
      def initialize(bool)
        @bool = bool
      end

      def to_s
        "push #{@bool}"
      end

      def generate(transform)
        return transform.push("TrueObject::the()") if @bool
        transform.push("FalseObject::the()")
      end

      def execute(vm)
        vm.push(@bool)
      end
    end
  end
end
