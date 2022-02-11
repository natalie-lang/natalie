require_relative './base_pass'

module Natalie
  class Compiler2
    # This compiler pass marks VariableGet and VariabletSet instructions as captured
    # if they are used in a block (closure). It also builds a 'env' hierarchy used by
    # the C++ backend to determine variable scope.
    class Pass2 < BasePass
      def initialize(instructions)
        @instructions = InstructionManager.new(instructions)
        @env = { vars: {}, outer: nil }
      end

      def transform
        @instructions.walk do |instruction|
          method = "transform_#{instruction.label}"
          method << "_#{instruction.matching_label}" if instruction.matching_label
          if respond_to?(method, true)
            send(method, instruction)
          end
          instruction.env = @env
        end
      end

      private

      def transform_define_block(_) @env = { vars: {}, outer: @env, block: true } end
      def transform_end_define_block(_) @env = @env[:outer] end

      def transform_define_class(_) @env = { vars: {}, outer: @env } end
      def transform_end_define_class(_) @env = @env[:outer] end

      def transform_define_method(_) @env = { vars: {}, outer: @env } end
      def transform_end_define_method(_) @env = @env[:outer] end

      def transform_if(_) @env = { vars: {}, outer: @env, hoist: true } end
      def transform_end_if(_) @env = @env[:outer] end

      def transform_try(_) @env = { vars: {}, outer: @env, hoist: true } end
      def transform_end_try(_) @env = @env[:outer] end

      def transform_variable_get(instruction)
        instruction.meta = find_or_create_var(instruction.name)
      end

      def transform_variable_set(instruction)
        instruction.meta = find_or_create_var(instruction.name, local_only: instruction.local_only)
      end

      def find_or_create_var(name, local_only: false)
        owning_env = @env

        # "hoisted" envs don't ever own a variable
        while owning_env[:hoist]
          hoisted_out_of_env = owning_env
          owning_env = owning_env.fetch(:outer)
        end

        env = owning_env

        # if a variable is not referenced from a block within this
        # scope, then it is not "captured".
        capturing = false

        loop do
          if env.fetch(:vars).key?(name)
            var = env.dig(:vars, name)
            var[:captured] = true if capturing
            return var
          end

          if env[:block] && !local_only && (outer = env.fetch(:outer))
            env = outer
            capturing = true
          else
            break
          end
        end

        var = {
          name: "#{name}_var",
          captured: false,
          declared: false,
          index: owning_env[:vars].size
        }

        owning_env[:vars][name] = var

        # IfInstruction and TryInstruction code generation (for the C++
        # backend) needs to declare variables just prior to the C++ scope
        # where the variables are created so that the resulting semantics
        # match Ruby. Thus, in addition to hoisting each variable up, we
        # need to keep track of where it emerged so that it can be properly
        # initialized to nil.
        if hoisted_out_of_env
          hoisted_out_of_env[:hoisted_vars] ||= {}
          hoisted_out_of_env[:hoisted_vars][name] ||= var
        end

        var
      end
    end
  end
end
