require_relative './base_pass'

module Natalie
  class Compiler2
    # This compiler pass sets needed break points on SendInstructions in order
    # to handle `break` from a block.
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
      end

      private

      def transform_break(instruction)
        raise 'TODO' unless @env[:block]
        break_point = (@break_point += 1)
        @env[:has_break] = break_point
        instruction.break_point = break_point
      end

      def transform_end_define_block(_)
        # TODO: how to handle procs/lambdas?
        if (break_point = @env[:has_break])
          _, send_instruction = @instructions.find_next(SendInstruction)
          send_instruction.break_point = break_point
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
