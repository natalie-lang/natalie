require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler
    # This compiler pass sets up needed break points to handle `break` from a block,
    # lambda, or while loop. You can debug this pass with the `-d p3` CLI flag.
    class Pass3 < BasePass
      def initialize(instructions)
        super()
        @instructions = instructions

        # We need to keep track of which 'container' the `break` is operating in.
        # If it's a `while` loop, the BreakInstruction needs to be converted to a
        # BreakOutInstruction, which operates differently.
        @to_convert_to_break_out = {}
        @break_instructions = {}

        # We have to match each break point with the appropriate SendInstruction or
        # CreateLambdaInstruction. Since method calls are chainable, the next
        # instruction is not necessarily the one that goes with the break point.
        # Thus, a stack is needed.
        @break_point_stack = []
      end

      def transform
        @instructions.walk do |instruction|
          method = "transform_#{instruction.class.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label
          @env = instruction.env
          if respond_to?(method, true)
            send(method, instruction)
          end
        end

        @to_convert_to_break_out.each do |break_point, while_instruction|
          @break_instructions[break_point].each do |ip, _break_instruction|
            break_out = BreakOutInstruction.new
            break_out.while_instruction = while_instruction
            @instructions.replace_at(ip, break_out)
          end
        end

        @instructions
      end

      private

      def transform_break(instruction)
        env = @env
        env = env[:outer] while env[:hoist] && !env[:while]
        raise 'unexpected env for break' unless env[:block] || env[:while]
        unless (break_point = env[:has_break])
          break_point = @instructions.next_break_point
          env[:has_break] = break_point
        end
        instruction.break_point = break_point
        @break_instructions[break_point] ||= {}
        @break_instructions[break_point][@instructions.ip - 1] = instruction
      end

      def transform_create_lambda(instruction)
        return unless (break_point = @break_point_stack.pop)

        instruction.break_point = break_point
      end

      def transform_end_define_block(_)
        @break_point_stack << @env[:has_break]
      end

      def transform_send(instruction)
        return unless instruction.with_block?
        return unless (break_point = @break_point_stack.pop)

        try_instruction = TryInstruction.new
        @instructions.insert_left(try_instruction)

        @instructions.insert_right([
          CatchInstruction.new,
          MatchBreakPointInstruction.new(break_point),
          IfInstruction.new,
          GlobalVariableGetInstruction.new(:$!),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :exit_value,
            receiver_is_self: false,
            with_block: false,
            file: instruction.file,
            line: instruction.line,
            receiver_pushed_first: true,
          ),
          ElseInstruction.new(:if),
          PushSelfInstruction.new,
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :raise,
            receiver_is_self: true,
            with_block: false,
            file: instruction.file,
            line: instruction.line,
            receiver_pushed_first: true,
          ),
          EndInstruction.new(:if),
          EndInstruction.new(:try),
        ])
      end

      # NOTE: `while` loops are different.
      # This code tracks break points independently from the above methods.
      def transform_end_while(instruction)
        return unless (break_point = @env[:has_break])

        while_instruction = instruction.matching_instruction

        unless while_instruction.is_a?(WhileInstruction)
          raise "unexpected instruction: #{while_instruction.inspect}"
        end

        @to_convert_to_break_out[break_point] = while_instruction
      end

      class << self
        def debug_instructions(instructions)
          instructions.each_with_index do |instruction, index|
            puts "#{index} #{instruction}"
          end
        end
      end
    end
  end
end
