module Natalie
  class VM
    class Block
      def initialize(vm:)
        @vm = vm
        @ip = vm.ip
        @captured_block = vm.block
        @captured_self = vm.self
        @parent_scope = vm.scope
      end

      attr_reader :vm

      def inspect
        "#<#{self.class.name}:0x#{object_id.to_s(16)}>"
      end

      def call(*args)
        to_proc.call(*args)
      end

      def [](*args)
        to_proc.call(*args)
      end

      def to_proc
        @lambda ||= lambda do |*args|
          vm.with_self(@captured_self) do
            scope = { vars: {}, parent: @parent_scope }
            vm.push_call(
              name: nil,
              return_ip: vm.ip,
              args: args,
              scope: scope,
              block: @captured_block
            )
            vm.ip = @ip
            begin
              vm.run
            ensure
              vm.ip = vm.pop_call.fetch(:return_ip)
            end
            vm.pop # result must be returned from proc
          end
        end
      end
    end
  end
end
