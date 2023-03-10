require_relative './cpp_backend/transform'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class CppBackend
      include StringToCpp

      def initialize(instructions, compiler_context:)
        @instructions = instructions
        @compiler_context = compiler_context
        @symbols = {}
        @inline_functions = {}
        @top = []
      end

      def generate
        string_of_cpp = transform_instructions
        out = merge_cpp_with_template(string_of_cpp)
        reindent(out)
      end

      private

      def transform_instructions
        transform = Transform.new(
          @instructions,
          top: @top,
          compiler_context: @compiler_context,
          symbols: @symbols,
          inline_functions: @inline_functions,
        )
        transform.transform
      end

      def merge_cpp_with_template(string_of_cpp)
        @compiler_context[:template]
          .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
          .sub('/*' + 'NAT_OBJ_INIT' + '*/') { obj_init_lines.join("\n") }
          .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
          .sub('/*' + 'NAT_EVAL_BODY' + '*/') { string_of_cpp }
      end

      def declarations
        [obj_declarations, declare_symbols, @top.join("\n")].join("\n\n")
      end

      def init_matter
        [
          init_symbols.join("\n"),
          set_dollar_zero_global_in_main_to_c,
          set_compiler_version_to_c,
        ].compact.join("\n\n")
      end

      def declare_symbols
        "static SymbolObject *#{symbols_var_name}[#{@symbols.size}] = {};"
      end

      def init_symbols
        @symbols.map do |name, index|
          "#{symbols_var_name}[#{index}] = SymbolObject::intern(#{string_to_cpp(name.to_s)}, #{name.to_s.bytesize});"
        end
      end

      def set_dollar_zero_global_in_main_to_c
        return if @compiler_context[:is_obj]

        "env->global_set(\"$0\"_s, new StringObject { #{@compiler_context[:source_path].inspect} });"
      end

      # NOTE: This is just used for tests and will likely be removed in the future.
      def set_compiler_version_to_c
        'GlobalEnv::the()->Object()->const_set("NATALIE_COMPILER"_s, new StringObject("v2"));'
      end

      def symbols_var_name
        "#{@compiler_context[:var_prefix]}symbols"
      end

      def obj_files
        rb_files = Dir.children(File.expand_path('../../../../src', __dir__)).grep(/^[a-z0-9_]+\.rb$/)
        list = rb_files.sort.map { |name| name.split('.').first }
        ['exception'] + # must come first
          (list - ['exception']) +
          @compiler_context[:required_cpp_files].values
      end

      def obj_declarations
        obj_files.map { |name| "Value init_#{name}(Env *env, Value self);" }.join("\n")
      end

      def obj_init_lines
        obj_files.map { |name| "init_#{name}(env, self);" }
      end

      def reindent(code)
        out = []
        indent = 0
        code
          .split("\n")
          .each do |line|
            indent -= 4 if line =~ /^\s*\}/
            indent = [0, indent].max
            if line.start_with?('#')
              out << line
            else
              out << line.sub(/^\s*/, ' ' * indent)
            end
            indent += 4 if line.end_with?('{')
          end
        out.join("\n")
      end
    end
  end
end
