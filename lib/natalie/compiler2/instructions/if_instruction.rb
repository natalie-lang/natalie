require_relative './base_instruction'

module Natalie
  class Compiler2
    class IfInstruction < BaseInstruction
      def to_s
        'if'
      end

      def has_body?
        true
      end

      def generate(transform)
        true_body = transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :if)
        false_body = transform.fetch_block_of_instructions(expected_label: :if)
        condition = transform.pop
        result = transform.temp('if_result')
        code = []
        code << "Value #{result}"
        code << "if (#{condition}->is_truthy()) {"
        stack_sizes = []
        transform.with_same_scope(true_body) do |t|
          code << t.transform("#{result} =")
          stack_sizes << t.stack.size
        end
        code << '} else {'
        transform.with_same_scope(false_body) do |t|
          code << t.transform("#{result} =")
          stack_sizes << t.stack.size
        end
        code << '}'
        # truncate resulting stack to minimum size of either branch's stack above
        stack_sizes << transform.stack.size
        transform.stack[stack_sizes.min..] = []
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        start_ip = vm.ip
        vm.skip_block_of_instructions(until_instruction: ElseInstruction, expected_label: :if)
        else_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :if)
        end_ip = vm.ip
        condition = vm.pop
        if condition
          vm.ip = start_ip
          vm.run
          vm.ip = end_ip
        else
          vm.ip = else_ip
          vm.run
          vm.ip = end_ip
        end
      end
    end
  end
end
