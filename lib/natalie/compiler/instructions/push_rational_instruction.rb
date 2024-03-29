require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRationalInstruction < BaseInstruction
      def to_s
        'push_rational'
      end

      def generate(transform)
        denominator = transform.pop
        numerator = transform.pop
        transform.exec_and_push(:rational, "Value(RationalObject::create(env, #{numerator}->to_int(env), #{denominator}->to_int(env)))")
      end

      def execute(vm)
        denominator = vm.pop
        numerator = vm.pop
        vm.push(Rational(numerator, denominator))
      end
    end
  end
end
