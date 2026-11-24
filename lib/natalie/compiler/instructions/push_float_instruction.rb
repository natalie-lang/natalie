require_relative './base_instruction'

module Natalie
  class Compiler
    class PushFloatInstruction < BaseInstruction
      def initialize(float)
        @float = float
      end

      attr_reader :float

      def to_s
        "push_float #{@float}"
      end

      def generate_llvm(transform)
        bits = [@float].pack('G').unpack1('H*')
        transform.push_call("call i64 @nat_llvm_float(double 0x#{bits})")
      end
    end
  end
end
