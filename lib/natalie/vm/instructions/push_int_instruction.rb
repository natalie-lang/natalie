module Natalie
  class VM
    class PushIntInstruction
      def initialize(int)
        @int = int
      end

      def to_s
        "push_int #{@int}"
      end

      def execute(vm)
        vm.push(@int)
      end
    end
  end
end
