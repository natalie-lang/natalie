require_relative './cpp_backend/transform'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class CppBackend
      include StringToCpp

      ROOT_DIR = File.expand_path('../../../../', __dir__)
      SRC_PATH = File.join(ROOT_DIR, 'src')
      MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.cpp'))
      OBJ_TEMPLATE = File.read(File.join(SRC_PATH, 'obj_unit.cpp'))

      CRYPT_LIBRARIES = RUBY_PLATFORM =~ /darwin/ ? [] : %w[-lcrypt]

      # When running `bin/natalie script.rb`, we use dynamic linking to speed things up.
      LIBRARIES_FOR_DYNAMIC_LINKING = %w[
        -lnatalie_base
        -lonigmo
      ] + CRYPT_LIBRARIES

      # When using the REPL or compiling a binary with the `-c` option,
      # we use static linking for compatibility.
      LIBRARIES_FOR_STATIC_LINKING = %w[
        -lnatalie
      ] + CRYPT_LIBRARIES

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        augment_compiler_context
        @symbols = {}
        @inline_functions = {}
        @top = []
      end

      attr_reader :cpp_path

      def compile_to_binary
        write_file
        cmd = compiler_command
        out = `#{cmd} 2>&1`
        File.unlink(@cpp_path) unless @compiler.keep_cpp? || $? != 0
        puts "cpp file path is: #{@cpp_path}" if @compiler.keep_cpp?
        warn out if out.strip != ''
        raise Compiler::CompileError, 'There was an error compiling.' if $? != 0
      end

      def write_file
        cpp = generate
        if @compiler.write_obj_path
          File.write(@compiler.write_obj_path, cpp)
        else
          temp_cpp = Tempfile.create('natalie.cpp')
          temp_cpp.write(cpp)
          temp_cpp.close
          @cpp_path = temp_cpp.path
        end
      end

      def compiler_command
        [
          cc,
          @compiler.build_flags,
          (@compiler.shared? ? '-fPIC -shared' : ''),
          @compiler.inc_paths,
          "-o #{@compiler.out_path}",
          '-x c++ -std=c++17',
          (cpp_path || 'code.cpp'),
          Compiler::LIB_PATHS.map { |path| "-L #{path}" }.join(' '),
          libraries.join(' '),
          @compiler.link_flags,
        ].map(&:to_s).join(' ')
      end

      private

      def generate
        string_of_cpp = transform_instructions
        out = merge_cpp_with_template(string_of_cpp)
        reindent(out)
      end

      def transform_instructions
        transform = Transform.new(
          @instructions,
          top:              @top,
          compiler_context: @compiler_context,
          symbols:          @symbols,
          inline_functions: @inline_functions,
        )
        transform.transform
      end

      def merge_cpp_with_template(string_of_cpp)
        template
          .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
          .sub('/*' + 'NAT_OBJ_INIT' + '*/') { init_object_files.join("\n") }
          .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
          .sub('/*' + 'NAT_EVAL_BODY' + '*/') { string_of_cpp }
      end

      def template
        if @compiler.write_obj_path
          OBJ_TEMPLATE.gsub(/OBJ_NAME/, obj_name)
        else
          MAIN_TEMPLATE
        end
      end

      def libraries
        if @compiler.dynamic_linking?
          LIBRARIES_FOR_DYNAMIC_LINKING
        else
          LIBRARIES_FOR_STATIC_LINKING
        end
      end

      def obj_name
        # FIXME: I don't like that this method "knows" how to ignore the build/generated directory
        # Maybe we need another arg to specify the init name...
        @compiler
          .write_obj_path
          .sub(/\.rb\.cpp/, '')
          .sub(%r{.*build/generated/}, '')
          .tr('/', '_')
      end

      def var_prefix
        if @compiler.write_obj_path
          "#{obj_name}_"
        elsif @compiler.repl
          "repl#{@compiler.repl_num}_"
        else
          ''
        end
      end

      def cc
        ENV['CXX'] || 'c++'
      end

      def declarations
        [
          object_file_declarations,
          symbols_declaration,
          files_declaration,
          @top.join("\n")
        ].join("\n\n")
      end

      def init_matter
        [
          init_symbols.join("\n"),
          init_dollar_zero_global,
        ].compact.join("\n\n")
      end

      def object_file_declarations
        object_files.map { |name| "Value init_#{name.tr('/', '_')}(Env *env, Value self);" }.join("\n")
      end

      def symbols_declaration
        "static SymbolObject *#{symbols_var_name}[#{@symbols.size}] = {};"
      end

      def files_declaration
        "static TM::Hashmap<SymbolObject *> #{files_var_name} {};"
      end

      def init_object_files
        object_files.map do |name|
          "init_#{name.tr('/', '_')}(env, self);"
        end
      end

      def init_symbols
        @symbols.map do |name, index|
          "#{symbols_var_name}[#{index}] = SymbolObject::intern(#{string_to_cpp(name.to_s)}, #{name.to_s.bytesize});"
        end
      end

      def init_dollar_zero_global
        return if @compiler_context[:is_obj]

        "env->global_set(\"$0\"_s, new StringObject { #{@compiler_context[:source_path].inspect} });"
      end

      def symbols_var_name
        static_var_name('symbols')
      end

      def files_var_name
        static_var_name('files')
      end

      def static_var_name(suffix)
        "#{@compiler_context[:var_prefix]}#{suffix}"
      end

      def object_files
        rb_files = Dir[File.expand_path('../../../../src/**/*.rb', __dir__)].map do |path|
          path.sub(%r{^.*/src/}, '')
        end.grep(%r{^([a-z0-9_]+/)?[a-z0-9_]+\.rb$})
        list = rb_files.sort.map { |name| name.split('.').first }
        ['exception'] + # must come first
          (list - ['exception']) +
          @compiler_context[:required_cpp_files].values
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

      def augment_compiler_context
        @compiler_context.merge!(
          var_prefix: var_prefix,
        )
      end
    end
  end
end
