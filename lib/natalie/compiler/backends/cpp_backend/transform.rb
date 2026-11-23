require_relative '../../native_build/code_writer'

module Natalie
  class Compiler
    class CppBackend
      class Transform < NativeBuild::CodeWriter
        def initialize(instructions, compiler_context:, transform_data:, stack: [], compiled_files: {})
          super(compiler_context:, transform_data:, stack:)
          if instructions.is_a?(InstructionManager)
            @instructions = instructions
          else
            @instructions = InstructionManager.new(instructions)
          end
          @result_stack = []
          @compiled_files = compiled_files
        end

        def ip
          @instructions.ip
        end

        attr_reader :compiled_files

        def transform(result_prefix = nil)
          @instructions.walk do |instruction|
            @env = instruction.env
            instruction.generate(self)
            @file = nil if instruction.is_a?(LoadFileInstruction)
          end
          @code << @stack.pop
          stringify_code(@code, result_prefix)
        end

        def find_var(name, local_only: false)
          env = @env
          depth = 0

          loop do
            env = env.fetch(:outer) while env[:hoist]
            if env.fetch(:vars).key?(name)
              var = env.dig(:vars, name)
              return depth, var
            end
            if env.fetch(:type) == :define_block && !local_only && (outer = env.fetch(:outer))
              env = outer
              depth += 1
            else
              break
            end
          end

          raise "unknown variable #{name}"
        end

        def fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: nil)
          @instructions.fetch_block(until_instruction: until_instruction, expected_label: expected_label)
        end

        def with_new_scope(instructions)
          t =
            Transform.new(
              instructions,
              transform_data: @transform_data,
              compiler_context: @compiler_context,
              compiled_files: @compiled_files,
            )
          yield(t)
        end

        def with_same_scope(instructions)
          stack = @stack.dup
          t =
            Transform.new(
              instructions,
              stack: stack,
              transform_data: @transform_data,
              compiler_context: @compiler_context,
              compiled_files: @compiled_files,
            )
          yield(t)
          @stack_sizes << stack.size if @stack_sizes
          stack.size
        end

        # truncate resulting stack to minimum size of any
        # code branch within this block
        def normalize_stack
          @stack_sizes = []
          yield
          @stack[@stack_sizes.min..] = []
          @stack_sizes = nil
        end

        def intern_param_table(parameters)
          index = @param_tables[parameters] ||= @param_tables.size
          "#{@var_prefix}param_table_#{index}"
        end

        def interned_string(str, encoding)
          index = @interned_strings[[str, encoding]] ||= @interned_strings.size
          "#{interned_strings_var_name}[#{index}]/*#{str.inspect.gsub(%r{/\*|\*/|\\}, '?')}*/"
        end

        def set_file(file)
          return unless file && file != @file
          @file = file
          exec("env->set_file(#{@file.inspect})")
        end

        def set_line(line)
          return unless line && line != @line
          @line = line
          exec("env->set_line(#{@line})")
        end

        def required_ruby_file(filename)
          @compiler_context[:required_ruby_files].fetch(filename)
        end

        def inspect
          "<#{self.class.name}:0x#{object_id.to_s(16)}>"
        end

        def has_file(name)
          "GlobalEnv::the()->has_file(#{name})"
        end

        def add_file(name)
          "GlobalEnv::the()->add_file(env, #{name})"
        end

        private

        def interned_strings_var_name
          "#{@var_prefix}interned_strings"
        end
      end
    end
  end
end
