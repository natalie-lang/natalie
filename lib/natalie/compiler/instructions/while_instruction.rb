require_relative './base_instruction'

module Natalie
  class Compiler
    class WhileInstruction < BaseInstruction
      def initialize(pre:)
        @pre = pre
      end

      attr_reader :pre

      def to_s
        s = 'while'
        s << ' (post)' unless pre
        s
      end

      def has_body?
        true
      end

      class << self
        def reset_result_id
          @result_id = 0
        end

        def next_result_name
          @result_id ||= 0
          "while_result#{@result_id += 1}"
        end
      end

      def result_name
        @result_name ||= self.class.next_result_name
      end

      def generate(transform)
        condition = transform.fetch_block_of_instructions(until_instruction: WhileBodyInstruction)
        body = transform.fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: :while)
        code = []

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = Value::nil()"
          var[:declared] = true
        end

        condition_name = transform.temp('while_condition')

        transform.with_same_scope(condition) do |t|
          code << "auto #{condition_name} = [&]() {"
          code << '  if (FiberObject::current()->check_redo_flag_and_clear()) { return Value::True(); }'
          code << t.transform('return')
          code << '};'
        end

        code << "Value #{result_name} = Value::nil()"

        transform.with_same_scope(body) do |t|
          if @pre
            code << "while (#{condition_name}().is_truthy()) {"
            code << t.transform
            code << '}'
          else
            code << 'do {'
            code << t.transform
            code << "} while (#{condition_name}().is_truthy())"
          end
        end

        transform.exec(code)
        transform.push(result_name)
      end
    end
  end
end
