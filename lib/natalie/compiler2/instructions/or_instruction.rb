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
        option_1 = transform.pop
        option_2 = transform.fetch_block_of_instructions(expected_label: :or)
        result = transform.temp("or")
        code = []
        code << "Value #{result} = #{option_1}"
        code << "if (!#{result}->is_truthy()) {"
        code << transform.with_same_scope(option_2) { |t| t.transform("#{result} =") }
        code << "}"
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :or)
        end_ip = vm.ip
        arg1 = vm.pop
        if arg1
          # We are already at end_ip
          vm.push arg1
        else
          vm.ip = ip
          vm.run # skip the end instruction
          # Afterwards we will be at the end of the or-statement
        end
      end
    end
  end
end
