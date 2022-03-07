require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler2
    # This compiler pass sets up needed break points to handle `break` from a block,
    # lambda, or while loop. You can debug this pass with the `-d p3` CLI flag.
    class Pass3 < BasePass
      def initialize(instructions)
        @instructions = InstructionManager.new(instructions)
        @break_point = 0

        # We need to keep track of which 'container' the `break` is operating in.
        # If it's a `while` loop, the BreakInstruction has to do extra work
        # to set the proper result variable (see BreakInstruction#generate).
        @break_point_instructions = {}
        @break_instructions = {}

        # We have to match each break point with the appropriate SendInstruction or
        # CreateLambdaInstruction. Since method calls are chainable, the next
        # instruction is not necessarily the one that goes with the break point.
        # Thus, a stack is needed.
        @break_point_stack = []
      end

      def transform
        @instructions.walk do |instruction|
          method = "transform_#{instruction.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label
          @env = instruction.env
          if respond_to?(method, true)
            send(method, instruction)
          end
        end

        @break_instructions.each do |break_point, break_instruction|
          break_instruction.break_point_instruction = @break_point_instructions[break_point]
        end

        # add envs to newly-added instructions
        EnvBuilder.new(@instructions).process
      end

      private

      def transform_break(instruction)
        env = @env
        env = env[:outer] while env[:hoist] && !env[:while]
        raise 'unexpected env for break' unless env[:block] || env[:while]
        break_point = (@break_point += 1)
        env[:has_break] = break_point
        instruction.break_point = break_point
        @break_instructions[break_point] = instruction
      end

      def transform_create_lambda(instruction)
        return unless (break_point = @break_point_stack.pop)

        instruction.break_point = break_point
        @break_point_instructions[break_point] = instruction
      end

      def transform_end_define_block(instruction)
        @break_point_stack << @env[:has_break]
      end

      def transform_send(instruction)
        return unless (break_point = @break_point_stack.pop)

        try_instruction = TryInstruction.new
        @break_point_instructions[break_point] = try_instruction
        @instructions.insert_left(try_instruction)

        @instructions.insert_right([
          CatchInstruction.new,
          MatchBreakPointInstruction.new(break_point),
          IfInstruction.new,
          PushArgcInstruction.new(0),
          GlobalVariableGetInstruction.new(:$!),
          SendInstruction.new(:exit_value, receiver_is_self: false, with_block: false, file: instruction.file, line: instruction.line),
          ElseInstruction.new(:if),
          PushArgcInstruction.new(0),
          PushNilInstruction.new,
          SendInstruction.new(:raise, receiver_is_self: false, with_block: false, file: instruction.file, line: instruction.line),
          EndInstruction.new(:if),
          EndInstruction.new(:try),
        ])
      end

      # NOTE: `while` loops are different.
      # This code tracks break points independently from the above methods.
      # FIXME: I would like a new instruction for the IR called BreakOutInstruction.
      # Breaking out of a while loop is quite different than other breaks.
      def transform_end_while(instruction)
        return unless (break_point = @env[:has_break])

        while_instruction = instruction.matching_instruction

        unless while_instruction.is_a?(WhileInstruction)
          raise "unexpected instruction: #{while_instruction.inspect}"
        end

        while_instruction.break_point = break_point
        @break_point_instructions[break_point] = while_instruction
      end

      def self.debug_instructions(instructions)
        instructions.each_with_index do |instruction, index|
          puts "#{index} #{instruction}"
        end
      end
    end
  end
end
