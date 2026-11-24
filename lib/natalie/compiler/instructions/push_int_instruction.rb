require_relative './base_instruction'

module Natalie
  class Compiler
    class PushIntInstruction < BaseInstruction
      def initialize(int)
        @int = int
      end

      attr_reader :int

      def to_s
        "push_int #{@int}"
      end

      def generate_llvm(transform)
        # The runtime owns the representation, including bignum promotion.
        transform.push_call("call i64 @nat_llvm_integer(ptr #{transform.string(@int.to_s)})")
      end
    end
  end
end
