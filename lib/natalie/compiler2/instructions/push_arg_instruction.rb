require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgInstruction < BaseInstruction
      def initialize(index, nil_default: false)
        @index = index
        @nil_default = nil_default
      end

      attr_reader :index, :nil_default

      def to_s
        s = "push_arg #{@index}"
        s << ' (nil default)' if @nil_default
        s
      end

      def generate(transform)
        if @nil_default
          transform.push("args.at(#{@index}, NilObject::the())")
        else
          transform.push("args[#{@index}]")
        end
      end

      def execute(vm)
        vm.push(vm.args[@index])
      end
    end
  end
end
