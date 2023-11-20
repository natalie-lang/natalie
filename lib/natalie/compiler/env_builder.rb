module Natalie
  class Compiler
    # This class builds the 'env' hierarchy used to determine variable scope.
    class EnvBuilder
      def initialize(instructions, env:)
        if instructions.is_a?(Array)
          @instructions = instructions
        else
          @instructions = instructions.to_a
        end
        @env = env
        @instruction_stack = []
        @ip = 0
      end

      def process
        while @ip < @instructions.size
          instruction = @instructions[@ip]
          method = "process_#{instruction.class.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label

          @instruction_stack << instruction if instruction.has_body?

          if instruction.is_a?(EndInstruction)
            unless (instruction.matching_instruction = @instruction_stack.pop)
              raise 'not enough stack'
            end
            instruction.env = @env
          end

          if respond_to?(method, true)
            send(method, instruction)
          end
          instruction.env ||= @env

          @ip += 1
        end
      end

      private

      def process_define_block(i) @env = i.env || { vars: {}, outer: @env, block: true } end
      def process_end_define_block(_)
        if @instructions[@ip + 1].is_a?(CreateLambdaInstruction)
          @env[:lambda] = true
        end
        @env = @env.fetch(:outer)
      end

      def process_define_class(i) @env = i.env || { vars: {}, outer: @env } end
      def process_end_define_class(_) @env = @env.fetch(:outer) end

      def process_define_method(i) @env = i.env || { vars: {}, outer: @env } end
      def process_end_define_method(_) @env = @env.fetch(:outer) end

      def process_define_module(i) @env = i.env || { vars: {}, outer: @env } end
      def process_end_define_module(_) @env = @env.fetch(:outer) end

      def process_if(i) @env = i.env || { vars: {}, outer: @env, hoist: true } end
      def process_end_if(_) @env = @env.fetch(:outer) end

      def process_try(i) @env = i.env || { vars: {}, outer: @env, hoist: true } end
      def process_end_try(_) @env = @env.fetch(:outer) end

      def process_while(i)
        return if i.env

        @env = {
          vars: {},
          outer: @env,
          hoist: true,
          while: true,
          result_name: i.result_name # used by BreakOutInstruction to set result variable
        }
      end

      def process_end_while(_) @env = @env.fetch(:outer) end

      def process_with_main(i) @env = i.env || { vars: {}, outer: @env, main: true } end
      def process_end_with_main(_) @env = @env.fetch(:outer) end
    end
  end
end
