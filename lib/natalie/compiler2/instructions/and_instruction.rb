require_relative './base_instruction'

module Natalie
  class Compiler2
    class AndInstruction < BaseInstruction
      def to_s
        'and'
      end

      def has_body?
        true
      end

      def generate(transform)
        rhs_body = transform.fetch_block_of_instructions(expected_label: :and)
        lhs = transform.temp('and_lhs')
        result = transform.temp('and_result')
        code = []
        code << "Value #{lhs} = #{transform.pop}"
        code << "Value #{result}"
        code << "if (#{lhs}->is_truthy()) {"
        code << transform.with_same_scope(rhs_body) { |t| t.transform("#{result} =") }
        code << '} else {'
        code << "#{result} = #{lhs}"
        code << '}'
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        rhs_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :and)
        end_ip = vm.ip
        lhs = vm.pop
        if lhs
          vm.ip = rhs_ip
          vm.run
          vm.ip = end_ip
        else
          vm.push(lhs)
        end
      end
    end
  end
end
