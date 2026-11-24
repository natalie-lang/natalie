require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateRationalInstruction < BaseInstruction
      def to_s
        'create_rational'
      end

      def generate_llvm(transform)
        denominator = transform.pop
        numerator = transform.pop
        transform.push_call("call i64 @nat_llvm_rational(ptr %env, i64 #{numerator}, i64 #{denominator})")
      end
    end
  end
end
