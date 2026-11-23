require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler
    # Set up scoped jumps for returns and redo across blocks and ensure handlers.
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
        # Check whether there is an ensure-protected `try` between us and the
        # enclosing method/block; if so, the return must throw so that the
        # ensure's catch handler runs (a bare C++ `return` would skip ensure).
        has_enclosing_ensure = false
        cur = @env
        until cur.nil? || %i[define_method define_block].include?(cur.fetch(:type, nil))
          has_enclosing_ensure ||= cur[:has_ensure]
          cur = cur[:outer]
        end

        env = @env
        env = env[:outer] while env[:hoist]

        if env.fetch(:type) != :define_block && !has_enclosing_ensure
          # ReturnInstruction inside anything else is OK as-is.
          return
        end

        # get the enclosing lambda or method
        # also mark every block in case it becomes a method later
        until env.fetch(:type) == :define_method || (env.fetch(:type) == :define_block && env[:for_lambda])
          env[:has_return] = true if env.fetch(:type) == :define_block
          env = env[:outer]
          break if env.nil?
          env = env[:outer] while env[:hoist]
        end

        # create a new break point and attach to env
        if env
          unless (break_point = env[:return_break_point])
            break_point = @instructions.next_break_point
            env[:return_break_point] = break_point
            if env[:for_lambda]
              # gotta save the break point for the CreateLambdaInstruction coming up
              @create_lambda_break_point = break_point
            end
          end
        else
          # special signal for any block converted to a method
          break_point = -1
        end

        # convert ReturnInstruction to BreakInstruction
        break_instruction = BreakInstruction.new(type: :return)
        break_instruction.break_point = break_point
        @instructions.replace_current([break_instruction])
      end

      # Like return, redo must unwind through intervening ensure handlers.
      # Catch the scoped jump at the block/loop boundary, then request replay.
      def transform_while(_)
        @env[:in_condition] = true
      end

      def transform_while_body(_)
        @env[:in_condition] = false
      end

      def transform_break_out(instruction)
        scope, crosses_ensure = loop_scope
        if crosses_ensure || scope[:in_condition]
          scope[:loop_break_point] ||= @instructions.next_break_point
          instruction.jump_point = scope.fetch(:loop_break_point)
        end
      end

      def transform_continue(instruction)
        scope, crosses_ensure = loop_scope
        if crosses_ensure || scope[:in_condition]
          scope[:loop_next_point] ||= @instructions.next_break_point
          instruction.jump_point = scope.fetch(:loop_next_point)
        end
      end

      def loop_scope
        scope = @env
        crosses_ensure = false
        until scope[:type] == :while
          crosses_ensure ||= scope[:has_ensure]
          scope = scope.fetch(:outer)
        end
        [scope, crosses_ensure]
      end

      def transform_next(instruction)
        scope = @env
        crosses_ensure = false
        until scope.nil? || scope[:type] == :define_block
          crosses_ensure ||= scope[:has_ensure]
          scope = scope[:outer]
        end
        return unless scope && crosses_ensure
        scope[:next_break_point] ||= @instructions.next_break_point
        instruction.jump_point = scope.fetch(:next_break_point)
      end

      def transform_redo(instruction)
        scope = @env
        has_ensure = false
        until scope.nil? || %i[define_block while].include?(scope[:type])
          has_ensure ||= scope[:has_ensure]
          scope = scope[:outer]
        end
        return unless scope && (has_ensure || scope[:in_condition])

        scope[:redo_break_point] ||= @instructions.next_break_point
        instruction.redo_point = scope.fetch(:redo_break_point)
      end

      def transform_end_while(instruction)
        instruction.matching_instruction.redo_point = @env[:redo_break_point]
        instruction.matching_instruction.break_point = @env[:loop_break_point]
        instruction.matching_instruction.next_point = @env[:loop_next_point]
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

      def transform_end_define_block(instruction)
        instruction.matching_instruction.has_return = true if @env[:has_return]
        instruction.matching_instruction.redo_point = @env[:redo_break_point]
        instruction.matching_instruction.next_point = @env[:next_break_point]
      end

      class << self
        def debug_instructions(instructions)
          instructions.each_with_index { |instruction, index| puts "#{index} #{instruction}" }
        end
      end
    end
  end
end
