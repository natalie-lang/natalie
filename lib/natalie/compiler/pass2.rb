require_relative './base_pass'
require_relative './env_builder'

module Natalie
  class Compiler
    # This compiler pass marks VariableGet and VariabletSet instructions as
    # captured if they are used in a block (closure).
    # You can debug this pass with the `-d p2` CLI flag.
    class Pass2 < BasePass
      def initialize(instructions, compiler_context:)
        super()
        @compiler_context = compiler_context
        env = { vars: compiler_context[:vars], outer: nil, type: :top }
        @instructions = InstructionManager.new(instructions, env:)
        EnvBuilder.new(@instructions, env:).process
      end

      def transform
        @instructions.walk do |instruction|
          method = "transform_#{instruction.class.label}"
          send(method, instruction) if respond_to?(method, true)
        end
      end

      private

      def transform_variable_declare(instruction)
        instruction.meta = find_or_create_var(instruction.env, instruction.name)
      end

      def transform_variable_get(instruction)
        instruction.meta = find_or_create_var(instruction.env, instruction.name)
      end

      def transform_variable_set(instruction)
        # We need to mark the variable as captured in the odd case of:
        #
        #     foo = foo
        #
        previous_instruction = @instructions.peek_back(2)
        setting_to_itself =
          previous_instruction.is_a?(VariableGetInstruction) && previous_instruction.name == instruction.name

        instruction.meta =
          find_or_create_var(
            instruction.env,
            instruction.name,
            local_only: instruction.local_only,
            setting_to_itself: setting_to_itself,
          )
      end

      def find_or_create_var(env, name, local_only: false, setting_to_itself: false)
        raise "bad var name: #{name.inspect}" unless name =~ /^(?:[[:alpha:]]|_)[[:alnum:]]*/

        owning_env = env

        # "hoisted" envs don't ever own a variable
        while owning_env[:hoist]
          hoisted_out_of_env = owning_env
          owning_env = owning_env.fetch(:outer)
        end

        env = owning_env

        # Two easy cases where the variable gets marked as "captured":
        #
        # 1. We're in the REPL
        # 2. We're setting the variable to itself
        capturing = repl? || setting_to_itself

        # And one complicated case where it gets marked:
        #
        # 3. The variable is referenced from a block
        loop do
          if env.fetch(:vars).key?(name)
            var = env.dig(:vars, name)
            var[:captured] = true if capturing
            return var
          end

          if env.fetch(:type) == :define_block && !local_only && (outer = env.fetch(:outer))
            env = outer
            capturing = true
            env = env.fetch(:outer) while env[:hoist]
          else
            break
          end
        end

        var = { name: "#{name}_var", captured: false, declared: false, index: owning_env[:vars].size }

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

      def repl?
        @compiler_context[:repl]
      end

      class << self
        def debug_instructions(instructions)
          env = nil
          instructions.each_with_index do |instruction, index|
            desc = "#{index} #{instruction}"
            puts desc if instruction.is_a?(EndInstruction)
            unless env.equal?(instruction.env)
              env = instruction.env
              e = env
              vars = e[:vars].keys.sort.map { |v| "#{v} (mine)" }
              while (e[:hoist] || e.fetch(:type) == :define_block) && (e = e.fetch(:outer))
                vars += e[:vars].keys.sort
              end
              puts
              puts '== SCOPE ' \
                     "vars=[#{vars.join(', ')}] " \
                     "#{env.fetch(:type) == :define_block ? 'is_block ' : ''}" \
                     '=='
            end
            puts desc unless instruction.is_a?(EndInstruction)
          end
        end
      end
    end
  end
end
