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
        transform.top("FlipFlopState #{state} = FlipFlopState::Off;")

        switch_on_body = transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :flip_flop)
        switch_off_body = transform.fetch_block_of_instructions(expected_label: :flip_flop)

        code = []
        transform.normalize_stack do
          switch_on_result = transform.temp('switch_on_result')
          switch_off_result = transform.temp('switch_on_result')

          code << "if (#{state} == FlipFlopState::On) {"
          transform.with_same_scope(switch_off_body) do |t|
            code << t.transform("Value #{switch_off_result} =")
          end
          code << "  if (#{switch_off_result}.is_truthy()) {"
          code << "    #{state} = FlipFlopState::Transitioning"
          code << '  }'
          code << '} else {'
          transform.with_same_scope(switch_on_body) do |t|
            code << t.transform("Value #{switch_on_result} =")
          end
          code << "  if (#{state} == FlipFlopState::Transitioning) {"
          code << "    #{state} = #{switch_on_result}.is_truthy() ? FlipFlopState::On : FlipFlopState::Off"
          code << "  } else if (#{switch_on_result}.is_truthy()) {"
          if @exclude_end
            code << "    #{state} = FlipFlopState::On"
          else
            transform.with_same_scope(switch_off_body) do |t|
              code << t.transform("Value #{switch_off_result} =")
            end
            code << "    #{state} = #{switch_off_result}.is_truthy() ? FlipFlopState::Transitioning : FlipFlopState::On"
          end
          code << '  }'
          code << '}'
        end

        transform.exec(code)
        transform.push("(#{state} == FlipFlopState::Off ? Value::False() : Value::True())")
      end

      def execute(vm)
        on_start_ip = vm.ip
        vm.skip_block_of_instructions(until_instruction: ElseInstruction, expected_label: :flip_flop)
        off_start_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :flip_flop)
        end_ip = vm.ip

        vm.flip_flop_states[on_start_ip] ||= :off

        state = vm.flip_flop_states[on_start_ip]
        if state == :on
          vm.ip = off_start_ip
          vm.run
          if vm.pop
            vm.flip_flop_states[on_start_ip] = :transitioning
          end
        else
          vm.ip = on_start_ip
          vm.run
          result = vm.pop
          if state == :transitioning
            vm.flip_flop_states[on_start_ip] = result ? :on : :off
          elsif result
            if @exclude_end
              vm.flip_flop_states[on_start_ip] = :on
            else
              vm.ip = off_start_ip
              vm.run
              vm.flip_flop_states[on_start_ip] = vm.pop ? :transitioning : :on
            end
          end
        end

        vm.ip = end_ip
        vm.push(vm.flip_flop_states[on_start_ip] != :off)
      end

      def serialize(_)
        [instruction_number, @exclude_end ? 1 : 0].pack('CC')
      end

      def self.deserialize(io, _)
        exclude_end = io.getbool
        new(exclude_end)
      end
    end
  end
end
