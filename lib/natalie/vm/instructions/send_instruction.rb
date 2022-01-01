module Natalie
  class VM
    class SendInstruction
      def initialize(message)
        @message = message
      end

      def to_s
        "send #{@message}"
      end

      def execute(vm)
        receiver = vm.pop
        arg_count = vm.pop
        args = []
        arg_count.times do
          args << vm.pop
        end
        receiver.send(@message, *args)
      end
    end
  end
end
