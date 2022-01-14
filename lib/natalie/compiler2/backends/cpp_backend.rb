require_relative './cpp_backend/transform'

module Natalie
  class Compiler2
    class CppBackend
      def initialize(instructions, compiler_context:)
        @instructions = instructions
        @compiler_context = compiler_context
        @source_files = { compiler_context[:source_path] => 0 }
        @symbols = {}
        @top = []
      end

      def generate
        string_of_cpp = transform_instructions
        out = merge_cpp_with_template(string_of_cpp)
        reindent(out)
      end

      private

      def transform_instructions
        transform = Transform.new(@instructions, top: @top, compiler_context: @compiler_context)
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
        [obj_declarations, source_files_to_c, declare_symbols, @top.join("\n")].join("\n\n")
      end

      def init_matter
        [init_symbols.join("\n"), set_dollar_zero_global_in_main_to_c].compact.join("\n")
      end

      def declare_symbols
        "SymbolObject *#{symbols_var_name}[#{@symbols.size}] = {};"
      end

      def init_symbols
        @symbols.map { |name, index| "#{symbols_var_name}[#{index}] = #{name.to_s.inspect}_s;" }
      end

      def set_dollar_zero_global_in_main_to_c
        return if @compiler_context[:is_obj]

        "env->global_set(\"$0\"_s, new StringObject { #{@compiler_context[:source_path].inspect} });"
      end

      def symbols_var_name
        "#{@compiler_context[:var_prefix]}symbols"
      end

      def obj_files
        rb_files = Dir.children(File.expand_path('../../../../src', __dir__)).grep(/\.rb$/)
        list = rb_files.sort.map { |name| name.split('.').first }
        ['exception'] + (list - ['exception'])
      end

      def obj_declarations
        obj_files.map { |name| "Value init_obj_#{name}(Env *env, Value self);" }.join("\n")
      end

      def obj_init_lines
        obj_files.map { |name| "init_obj_#{name}(env, self);" }
      end

      def source_files_to_c
        files = "#{@compiler_context[:var_prefix]}source_files"
        "const char *#{files}[#{@source_files.size}] = { #{@source_files.keys.map(&:inspect).join(', ')} };"
      end

      def reindent(code)
        out = []
        indent = 0
        code
          .split("\n")
          .each do |line|
            indent -= 4 if line =~ /^\s*\}/
            indent = [0, indent].max
            out << line.sub(/^\s*/, ' ' * indent)
            indent += 4 if line.end_with?('{')
          end
        out.join("\n")
      end
    end
  end
end
