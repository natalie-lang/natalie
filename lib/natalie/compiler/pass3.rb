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

        @instructions.replace_current(
          [
            try_instruction,
            instruction,
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
            ),
            EndInstruction.new(:if),
            EndInstruction.new(:try),
          ]
        )
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
