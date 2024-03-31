require_relative './base_instruction'

module Natalie
  class Compiler
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

      def serialize(_)
        [
          instruction_number,
          index,
          nil_default ? 1 : 0,
        ].pack("CwC")
      end

      def self.deserialize(io, _)
        index = io.read_ber_integer
        nil_default = io.getbool
        new(index, nil_default: nil_default)
      end
    end
  end
end
