require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler
    # This compiler pass sets up needed break points to handle `return` from a block.
    # You can debug this pass with the `-d p4` CLI flag.
    class Pass4 < BasePass
      def initialize(instructions, **)
        super()
        @instructions = instructions
      end

      def transform
        @instructions.walk do |instruction|
          method = "transform_#{instruction.class.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label
          @env = instruction.env
          send(method, instruction) if respond_to?(method, true)
        end
      end

      private

      def transform_return(instruction)
        env = @env
        env = env[:outer] while env[:hoist]

        unless env.fetch(:type) == :define_block
          # ReturnInstruction inside anything else is OK as-is.
          return
        end

        # get the enclosing lambda or method
        until env.fetch(:type) == :define_method || (env.fetch(:type) == :define_block && env[:for_lambda])
          env = env[:outer]
          return if env.nil? # top-level return, nothing to do
          env = env[:outer] while env[:hoist]
        end

        # create a new break point and attach to env
        unless (break_point = env[:return_break_point])
          break_point = @instructions.next_break_point
          env[:return_break_point] = break_point
          if env[:for_lambda]
            # gotta save the break point for the CreateLambdaInstruction coming up
            @create_lambda_break_point = break_point
          end
        end

        # convert ReturnInstruction to BreakInstruction
        break_instruction = BreakInstruction.new(type: :return)
        break_instruction.break_point = break_point
        @instructions.replace_current([break_instruction])
      end

      def transform_end_define_method(instruction)
        define_method_instruction = instruction.matching_instruction
        if define_method_instruction.env[:return_break_point]
          define_method_instruction.break_point = @env[:return_break_point]
        end
      end

      def transform_create_lambda(instruction)
        return unless @create_lambda_break_point

        instruction.break_point = @create_lambda_break_point
        @create_lambda_break_point = nil
      end

      class << self
        def debug_instructions(instructions)
          instructions.each_with_index { |instruction, index| puts "#{index} #{instruction}" }
        end
      end
    end
  end
end
