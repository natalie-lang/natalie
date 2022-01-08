module Natalie
  class VM
    class PushSelfInstruction
      def to_s
        'push_self'
      end

      def execute(vm)
        vm.push(vm.self)
      end
    end
  end
end
