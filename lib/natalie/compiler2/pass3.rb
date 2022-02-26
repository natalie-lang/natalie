require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler2
    # This compiler pass sets up needed break points to handle `break` from a block or lambda.
    # You can debug this pass with the `-d p3` CLI flag.
    class Pass3 < BasePass
      def initialize(instructions)
        @instructions = InstructionManager.new(instructions)
        @break_point = 0
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
        # add envs to newly-added instructions
        EnvBuilder.new(@instructions).process
      end

      private

      def transform_break(instruction)
        env = @env
        env = env[:outer] while env[:hoist]
        raise 'unexpected env for break' unless env[:block]
        break_point = (@break_point += 1)
        env[:has_break] = break_point
        instruction.break_point = break_point
      end

      def transform_end_define_block(_)
        # TODO: how to handle procs/lambdas?
        if (break_point = @env[:has_break])
          if @instructions.peek.is_a?(CreateLambdaInstruction)
            @instructions.peek.break_point = break_point
            return
          end
          @instructions.insert_after(TryInstruction.new)
          ip, send_instruction = @instructions.find_next(SendInstruction)
          @instructions.insert_at(ip + 1, [
            CatchInstruction.new,
            MatchBreakPointInstruction.new(break_point),
            IfInstruction.new,
            PushArgcInstruction.new(0),
            GlobalVariableGetInstruction.new(:$!),
            SendInstruction.new(:exit_value, receiver_is_self: false, with_block: false),
            ElseInstruction.new(:if),
            PushArgcInstruction.new(0),
            PushNilInstruction.new,
            SendInstruction.new(:raise, receiver_is_self: false, with_block: false),
            EndInstruction.new(:if),
            EndInstruction.new(:try),
          ])
        end
      end

      def self.debug_instructions(instructions)
        instructions.each_with_index do |instruction, index|
          puts "#{index} #{instruction}"
        end
      end
    end
  end
end
