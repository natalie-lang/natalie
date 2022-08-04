require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler2
    # This compiler pass sets up needed break points to handle `break` and `return`
    # statements. You can debug this pass with the `-d p3` CLI flag.
    #
    # There are really about 4 separate actions being taken here:
    #
    # == `return` inside a block ==
    # 1. Replace the ReturnInstruction with a BreakInstruction.
    # 2. Set up the needed break point try/catch around the appropriate SendInstruction.
    #    (Not necessarily the SendInstruction with the block; could be higher up.)
    # 3. When the break point is caught, return the exit_value from the enclosing method.
    #
    # == `break` inside a block ==
    # 1. Set up the needed break point try/catch around the SendInstruction for that block.
    # 2. When the break point is caught, set the result of the SendInstruction to the exit_value.
    #
    # == `break` inside a lambda ==
    # 1. Set a break point on the Lambda (ProcObject) so it can try/catch around the block execution.
    #    (It is the responsibility of the ProcObject to catch the break point and return
    #    from the lambda with the exit_value; See `ProcObject::call()`.)
    #
    # == `break` inside a while loop ==
    # 1. Replace the BreakInstruction with a BreakOutInstruction.
    #    (The BreakOutInstruction is responsible for setting the result of the while loop.)
    #
    # from a block, lambda, or while loop.
    class Pass3 < BasePass
      def initialize(instructions)
        @instructions = instructions
        @break_point = 0

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
      end

      private

      def transform_break(instruction)
        env = @env
        env = env[:outer] while env[:hoist] && !env[:while]
        raise 'unexpected env for break' unless env[:block] || env[:while]

        if env[:while]
          break_out_instruction = BreakOutInstruction.new
          while_instruction = @instructions.find_previous(WhileInstruction)
          break_out_instruction.while_instruction = while_instruction
          @instructions.replace_current(break_out_instruction)
          return
        end

        unless (break_point = env[:has_break])
          break_point = (@break_point += 1)
          env[:has_break] = break_point
        end
        instruction.break_point = break_point
      end

      def transform_create_lambda(instruction)
        return unless (break_point_env = @break_point_stack.pop)
        return unless (break_point = break_point_env[:has_break])

        instruction.break_point = break_point
      end

      def transform_end_define_block(_)
        @break_point_stack << @env
      end

      def transform_return(instruction)
        unless instruction.env
          # we just now added the ReturnInstruction, so it's fine :-)
          return
        end

        env = @env
        env = env[:outer] while env[:hoist]

        unless env[:block]
          # ReturnInstruction inside anything else is OK.
          return
        end

        # get the top-most block in the method
        top_block_env = env
        while top_block_env.dig(:outer, :block) || top_block_env[:hoist]
          top_block_env = top_block_env[:outer]
        end

        unless (break_point = top_block_env[:has_return_break])
          break_point = (@break_point += 1)
          top_block_env[:has_return_break] = break_point
        end

        break_instruction = BreakInstruction.new
        break_instruction.break_point = break_point
        @instructions.replace_current(break_instruction)
      end

      def transform_send(instruction)
        return unless instruction.with_block?
        return unless (break_point_env = @break_point_stack.pop)

        if break_point_env[:has_break]
          wrap_send(
            instruction,
            break_point: break_point_env[:has_break]
          )
        end

        if break_point_env[:has_return_break]
          wrap_send(
            instruction,
            break_point: break_point_env[:has_return_break],
            return_break: true
          )
        end
      end

      def wrap_send(instruction, break_point:, return_break: false)
        @instructions.insert_left(TryInstruction.new)
        @instructions.insert_right([
          CatchInstruction.new,
          MatchBreakPointInstruction.new(break_point),
          IfInstruction.new,
          PushArgcInstruction.new(0),
          GlobalVariableGetInstruction.new(:$!),
          SendInstruction.new(:exit_value, receiver_is_self: false, with_block: false, file: instruction.file, line: instruction.line),
          return_break ? ReturnInstruction.new : nil,
          ElseInstruction.new(:if),
          PushArgcInstruction.new(0),
          PushSelfInstruction.new,
          SendInstruction.new(:raise, receiver_is_self: true, with_block: false, file: instruction.file, line: instruction.line),
          EndInstruction.new(:if),
          EndInstruction.new(:try),
        ].compact)
      end

      def self.debug_instructions(instructions)
        instructions.each_with_index do |instruction, index|
          puts "#{index} #{instruction}"
        end
      end
    end
  end
end
