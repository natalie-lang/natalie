module Natalie
  class Compiler
    class CppBackend
      class Transform
        def initialize(instructions, top:, compiler_context:, symbols:, inline_functions:, stack: [], compiled_files: {})
          if instructions.is_a?(InstructionManager)
            @instructions = instructions
          else
            @instructions = InstructionManager.new(instructions)
          end
          @result_stack = []
          @top = top
          @code = []
          @compiler_context = compiler_context
          @symbols = symbols
          @inline_functions = inline_functions
          @stack = stack
          @compiled_files = compiled_files
        end

        def ip
          @instructions.ip
        end

        attr_reader :stack, :env, :compiled_files

        attr_accessor :inline_functions

        def transform(result_prefix = nil)
          @instructions.walk do |instruction|
            @env = instruction.env
            instruction.generate(self)
          end
          @code << @stack.pop
          stringify_code(@code, result_prefix)
        end

        def semicolon(line)
          line.strip =~ /^\#|[{};]$/ ? line : "#{line};"
        end

        def exec(code)
          @code << stringify_code(code)
        end

        def exec_and_push(name, code)
          result = memoize(name, code)
          push(result)
          result
        end

        def memoize(name, code)
          result = temp(name)
          @code << stringify_code(code, "auto #{result} = ")
          result
        end

        def push(result)
          @stack << result
        end

        def push_nil
          @stack << 'Value(NilObject::the())'
        end

        def pop
          raise 'ran out of stack' unless @stack.any?

          @stack.pop
        end

        def peek
          raise 'ran out of stack' unless @stack.any?

          @stack.last
        end

        def find_var(name, local_only: false)
          env = @env
          depth = 0

          loop do
            env = env.fetch(:outer) while env[:hoist]
            if env.fetch(:vars).key?(name)
              var = env.dig(:vars, name)
              return [depth, var]
            end
            if env[:block] && !local_only && (outer = env.fetch(:outer))
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

        def temp(name)
          name = name.to_s.gsub(/[^a-zA-Z_]/, '')
          n = @compiler_context[:var_num] += 1
          "#{@compiler_context[:var_prefix]}#{name}#{n}"
        end

        def with_new_scope(instructions)
          t = Transform.new(
            instructions,
            top: @top,
            compiler_context: @compiler_context,
            symbols: @symbols,
            inline_functions: @inline_functions,
            compiled_files: @compiled_files,
          )
          yield(t)
        end

        def with_same_scope(instructions)
          stack = @stack.dup
          t = Transform.new(
            instructions,
            stack: stack,
            top: @top,
            compiler_context: @compiler_context,
            symbols: @symbols,
            inline_functions: @inline_functions,
            compiled_files: @compiled_files,
          )
          yield(t)
          @stack_sizes << stack.size if @stack_sizes
        end

        # truncate resulting stack to minimum size of any
        # code branch within this block
        def normalize_stack
          @stack_sizes = []
          yield
          @stack_sizes << stack.size
          @stack[@stack_sizes.min..] = []
          @stack_sizes = nil
        end

        def top(code)
          @top << Array(code).join("\n")
        end

        def intern(symbol)
          index = @symbols[symbol] ||= @symbols.size
          comment = "/*:#{symbol.to_s.gsub(%r{\*/|\\}, '?')}*/"
          "#{symbols_var_name}[#{index}]#{comment}"
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

        def add_cxx_flags(flags)
          @compiler_context[:compile_cxx_flags] << flags
        end

        def add_ld_flags(flags)
          @compiler_context[:compile_ld_flags] << flags
        end

        def inspect
          "<#{self.class.name}:0x#{object_id.to_s(16)}>"
        end

        def files_var_name
          "#{@compiler_context[:var_prefix]}files"
        end

        private

        def value_has_side_effects?(value)
          return false if value =~ /^Value\((False|Nil|True)Object::the\(\)\)$/
          return false if value =~ /^Value\(symbols\[\d+\]\/\*\:[^\*]+\*\/\)$/
          true
        end

        def symbols_var_name
          "#{@compiler_context[:var_prefix]}symbols"
        end

        def stringify_code(lines, result_prefix = nil)
          lines = Array(lines).compact
          out = []
          while lines.any?
            line = lines.shift
            next if line.nil?
            next if result_prefix.nil? && !value_has_side_effects?(line)
            if lines.empty? && line !~ /^\s*env->raise|^\s*break;?$/
              out << "#{result_prefix} #{semicolon(line)}"
            else
              out << semicolon(line)
            end
          end
          out.join("\n")
        end

      end
    end
  end
end
