require_relative '../native_build'

module Natalie
  class Compiler
    module NativeBuild
      # C++ expression rendering shared by native macros and the legacy backend.
      class CodeWriter
        def initialize(compiler_context:, transform_data:, stack: [])
          @code = []
          @compiler_context = compiler_context
          @stack = stack
          @transform_data = transform_data
          @top = transform_data.top
          @symbols = transform_data.symbols
          @interned_strings = transform_data.interned_strings
          @inline_functions = transform_data.inline_functions
          @param_tables = transform_data.param_tables
          @var_prefix = transform_data.var_prefix
        end

        attr_reader :stack, :env
        attr_accessor :inline_functions

        def semicolon(line)
          line.strip =~ /^\#|[{};]$/ ? line : "#{line};"
        end

        def exec(code)
          @code << stringify_code(code)
        end

        def exec_and_push(name, code, type: 'Value')
          result = memoize(name, code, type:)
          push(result)
          result
        end

        def memoize(name, code, type: 'Value')
          result = temp(name)
          @code << stringify_code(code, "#{type} #{result} =")
          result
        end

        def push(result)
          @stack << result
        end

        def push_nil
          @stack << 'Value::nil()'
        end

        def pop
          raise 'ran out of stack' unless @stack.any?

          @stack.pop
        end

        def peek
          raise 'ran out of stack' unless @stack.any?

          @stack.last
        end

        def temp(name)
          name = name.to_s.gsub(/[^a-zA-Z_]/, '')
          n = @transform_data.next_var_num
          "#{@var_prefix}#{name}#{n}"
        end

        def get_top = @top

        def top(name, code)
          @top[name] = Array(code).join("\n")
        end

        def intern(symbol)
          index = @symbols[symbol] ||= @symbols.size
          comment = "/*:#{symbol.to_s.gsub(%r{\*/|\\}, '?')}*/"
          "#{symbols_var_name}[#{index}]#{comment}"
        end

        def add_cxx_flags(flags)
          @compiler_context[:compile_cxx_flags] << flags
        end

        def add_ld_flags(flags)
          @compiler_context[:compile_ld_flags] << flags
        end

        private

        def value_has_side_effects?(value)
          return false if value =~ /^Value\((False|Nil|True)Object::the\(\)\)$/
          return false if value =~ %r{^Value\(\w*symbols\[\d+\]/\*\:[^\*]+\*/\)$}
          true
        end

        def symbols_var_name
          "#{@var_prefix}symbols"
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
