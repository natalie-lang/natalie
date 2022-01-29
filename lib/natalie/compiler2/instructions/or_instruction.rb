require_relative './base_instruction'

module Natalie
  class Compiler2
    class OrInstruction < BaseInstruction
      def to_s
        'or'
      end

      def has_body?
        true
      end

      def generate(transform)
        rhs_body = transform.fetch_block_of_instructions(expected_label: :or)
        lhs = transform.temp('or_lhs')
        result = transform.temp('or_result')
        code = []
        code << "Value #{lhs} = #{transform.pop}"
        code << "Value #{result}"
        code << "if (#{lhs}->is_truthy()) {"
        code << "#{result} = #{lhs}"
        code << '} else {'
        code << transform.with_same_scope(rhs_body) { |t| t.transform("#{result} =") }
        code << '}'
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        rhs_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :or)
        end_ip = vm.ip
        lhs = vm.pop
        if lhs
          vm.push(lhs)
        else
          vm.ip = rhs_ip
          vm.run
          vm.ip = end_ip
        end
      end
    end
  end
end
