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

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_arg(ptr %args, i64 #{@index})")
      end
    end
  end
end
