require_relative './base_instruction'

module Natalie
  class Compiler
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

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = NilObject::the()"
          var[:declared] = true
        end

        code << "Value #{result}"

        transform.normalize_stack do
          code << "if (#{condition}->is_truthy()) {"
          if_size = transform.with_same_scope(true_body) do |t|
            code << t.transform("#{result} =")
          end

          code << '} else {'
          else_size = transform.with_same_scope(false_body) do |t|
            code << t.transform("#{result} =")
          end
          code << '}'

          if if_size != else_size
            puts 'code:'
            puts code
            puts
            puts 'true body:'
            puts true_body
            puts
            puts 'false body:'
            puts false_body
            raise "IfInstruction branch sizes are uneven! #{if_size} != #{else_size}\n" \
                  'This is a bug in the compiler. Please report it.'
          end
        end

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
          result = vm.run
          vm.ip = end_ip
        else
          vm.ip = else_ip
          result = vm.run
          vm.ip = end_ip
        end
        unless result == :halt
          result
        end
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
