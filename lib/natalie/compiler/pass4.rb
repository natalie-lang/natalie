require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler
    # This compiler pass sets up needed break points to handle `return` from a block.
    # You can debug this pass with the `-d p4` CLI flag.
    class Pass4 < BasePass
      def initialize(instructions)
        @instructions = instructions
        @break_point = 10_000 # FIXME

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
        while top_block_env[:hoist] || (
            top_block_env.dig(:outer, :block) &&
            !top_block_env[:lambda]
        )
          top_block_env = top_block_env[:outer]
        end

        unless (break_point = top_block_env[:has_return])
          break_point = (@break_point += 1)
          top_block_env[:has_return] = break_point
        end

        break_instruction = BreakInstruction.new
        break_instruction.break_point = break_point
        @instructions.replace_current(break_instruction)
      end

      def transform_create_lambda(instruction)
        return unless (break_point = @break_point_stack.pop)

        instruction.break_point = break_point
      end

      def transform_end_define_block(instruction)
        @break_point_stack << @env[:has_return]
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
          PushArgcInstruction.new(0),
          GlobalVariableGetInstruction.new(:$!),
          SendInstruction.new(:exit_value, receiver_is_self: false, with_block: false, file: instruction.file, line: instruction.line),
          ReturnInstruction.new,
          ElseInstruction.new(:if),
          PushArgcInstruction.new(0),
          PushSelfInstruction.new,
          SendInstruction.new(:raise, receiver_is_self: true, with_block: false, file: instruction.file, line: instruction.line),
          EndInstruction.new(:if),
          EndInstruction.new(:try),
        ])
      end

      def self.debug_instructions(instructions)
        instructions.each_with_index do |instruction, index|
          puts "#{index} #{instruction}"
        end
      end
    end
  end
end
