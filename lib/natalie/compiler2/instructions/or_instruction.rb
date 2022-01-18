require_relative './base_instruction'

module Natalie
  class Compiler2
    class OrInstruction < BaseInstruction
      def to_s
        'or'
      end

      def generate(transform)
        rhs = transform.pop
        lhs = transform.temp('lhs')
        transform.exec("auto #{lhs} = #{transform.pop}")
        transform.exec_and_push(:or, "(#{lhs}->is_truthy() ? #{lhs} : #{rhs})")
      end

      def execute(vm)
        rhs = vm.pop
        lhs = vm.pop
        vm.push(lhs || rhs)
      end
    end
  end
end
