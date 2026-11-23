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

      def generate(transform)
        denominator = transform.pop
        numerator = transform.pop
        transform.exec_and_push(
          :rational,
          "Value(RationalObject::create(env, #{numerator}.to_int(env), #{denominator}.to_int(env)))",
        )
      end
    end
  end
end
