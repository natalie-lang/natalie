module Natalie
  class Compiler2
    # This class builds the 'env' hierarchy used by the C++ backend to determine variable scope.
    class EnvBuilder
      def initialize(instructions)
        @instructions = InstructionManager.new(instructions)
        @env = { vars: {}, outer: nil }
        @instruction_stack = []
      end

      def process
        @instructions.walk do |instruction|
          method = "process_#{instruction.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label

          @instruction_stack << instruction if instruction.has_body?

          if instruction.is_a?(EndInstruction)
            instruction.matching_instruction = @instruction_stack.pop
            instruction.env = @env
          end

          if respond_to?(method, true)
            send(method, instruction)
          end
          instruction.env ||= @env
        end
      end

      private

      def process_define_block(_) @env = { vars: {}, outer: @env, block: true } end
      def process_end_define_block(_) @env = @env[:outer] end

      def process_define_class(_) @env = { vars: {}, outer: @env } end
      def process_end_define_class(_) @env = @env[:outer] end

      def process_define_method(_) @env = { vars: {}, outer: @env } end
      def process_end_define_method(_) @env = @env[:outer] end

      def process_if(_) @env = { vars: {}, outer: @env, hoist: true } end
      def process_end_if(_) @env = @env[:outer] end

      def process_try(_) @env = { vars: {}, outer: @env, hoist: true } end
      def process_end_try(_) @env = @env[:outer] end

      def process_while(_) @env = { vars: {}, outer: @env, hoist: true, while: true } end
      def process_end_while(_) @env = @env[:outer] end

      def process_with_self(_) @env = { vars: {}, outer: @env } end
      def process_end_with_self(_) @env = @env[:outer] end
    end
  end
end
