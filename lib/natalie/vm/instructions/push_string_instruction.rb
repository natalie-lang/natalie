module Natalie
  class VM
    class PushStringInstruction
      def initialize(string)
        @string = string
      end

      def to_s
        "push_string #{@string.inspect}"
      end

      def execute(vm)
        vm.push(@string)
      end
    end
  end
end
