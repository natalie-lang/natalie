require_relative './base_instruction'

module Natalie
  class Compiler
    class FlipFlopInstruction < BaseInstruction
      def initialize(exclude_end:)
        @exclude_end = exclude_end
        super()
      end

      def has_body?
        true
      end

      def to_s
        "flip_flop #{@exclude_end ? 'exclude_end' : ''}"
      end

      def generate(transform)
        state = transform.temp('flip_flop')
        transform.top(state, "FlipFlopState #{state} = FlipFlopState::Off;")

        switch_on_body =
          transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :flip_flop)
        switch_off_body = transform.fetch_block_of_instructions(expected_label: :flip_flop)

        code = []
        transform.normalize_stack do
          switch_on_result = transform.temp('switch_on_result')
          switch_off_result = transform.temp('switch_on_result')

          code << "if (#{state} == FlipFlopState::On) {"
          transform.with_same_scope(switch_off_body) { |t| code << t.transform("Value #{switch_off_result} =") }
          code << "  if (#{switch_off_result}.is_truthy()) {"
          code << "    #{state} = FlipFlopState::Transitioning"
          code << '  }'
          code << '} else {'
          transform.with_same_scope(switch_on_body) { |t| code << t.transform("Value #{switch_on_result} =") }
          code << "  if (#{state} == FlipFlopState::Transitioning) {"
          code << "    #{state} = #{switch_on_result}.is_truthy() ? FlipFlopState::On : FlipFlopState::Off"
          code << "  } else if (#{switch_on_result}.is_truthy()) {"
          if @exclude_end
            code << "    #{state} = FlipFlopState::On"
          else
            transform.with_same_scope(switch_off_body) { |t| code << t.transform("Value #{switch_off_result} =") }
            code << "    #{state} = #{switch_off_result}.is_truthy() ? FlipFlopState::Transitioning : FlipFlopState::On"
          end
          code << '  }'
          code << '}'
        end

        transform.exec(code)
        transform.push("(#{state} == FlipFlopState::Off ? Value::False() : Value::True())")
      end
    end
  end
end
