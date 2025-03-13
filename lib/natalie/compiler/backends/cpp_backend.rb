require_relative './cpp_backend/linker'
require_relative './cpp_backend/out_file'
require_relative './cpp_backend/transform'
require_relative '../string_to_cpp'
require_relative '../flags'

module Natalie
  class Compiler
    class CppBackend
      include StringToCpp
      include Flags

      ROOT_DIR = File.expand_path('../../../../', __dir__)
      BUILD_DIR = File.join(ROOT_DIR, 'build')
      SRC_PATH = File.join(ROOT_DIR, 'src')
      MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main_template.cpp'))
      OBJ_TEMPLATE = File.read(File.join(SRC_PATH, 'obj_unit_template.cpp'))
      LOADED_FILE_TEMPLATE = File.read(File.join(SRC_PATH, 'loaded_file_template.cpp'))

      DARWIN = RUBY_PLATFORM.match?(/darwin/)
      OPENBSD = RUBY_PLATFORM.match?(/openbsd/)

      DL_EXT = RbConfig::CONFIG['DLEXT']

      INC_PATHS = [
        File.join(ROOT_DIR, 'include'),
        File.join(ROOT_DIR, 'ext/tm/include'),
        File.join(BUILD_DIR),
        File.join(BUILD_DIR, 'onigmo/include'),
      ].freeze

      # When running `bin/natalie script.rb`, we use dynamic linking to speed things up.
      LIBRARIES_FOR_DYNAMIC_LINKING = %w[
        -lnatalie_base
        -lonigmo
      ].freeze

      # When compiling a binary with the `-c` option, we use static linking for compatibility.
      LIBRARIES_FOR_STATIC_LINKING = %w[
        -lnatalie
      ].freeze

      LIB_PATHS = [
        BUILD_DIR,
        File.join(BUILD_DIR, 'onigmo/lib'),
        File.join(BUILD_DIR, 'zlib'),
      ].freeze

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        augment_compiler_context
        @symbols = {}
        @interned_strings = {}
        @inline_functions = {}
        check_build
      end

      attr_reader :cpp_path

      def compile_to_binary
        outs = prepare_out_files
        object_paths = outs.map do |out|
          puts "compiling #{out.ruby_path}..."
          out.compile_object_file
        end
        puts 'linking...'
        link(object_paths)
        puts 'done'
      end

      def write_files_for_debugging
        prepare_out_files.map { |out| out.write_source_to_tempfile }
      end

      # def compile_to_binary
      #   prepare_temp
      #   compile_temp_to_binary
      # end
      #
      # def prepare_temp
      #   check_build
      #   write_file
      # end
      #
      # def compile_temp_to_binary
      #   object_file_path = @out_file.compile_object_file
      #   link([object_file_path])
      # end
      #
      # def write_object_source(path)
      #   @out_file = build_main_out_file
      #   @out_file.write_source_to_path(path)
      # end
      #
      # def write_file_for_debugging
      #   write_file
      #   @cpp_path
      # end
      #
      # def compiler_command
      #   @out_file = build_main_out_file
      #   @out_file.compiler_command
      #   [
      #     @out_file.compiler_command,
      #     linker([@out_file.out_path]).link_command,
      #   ].join("\n")
      # end

      private

      def prepare_out_files
        outs = []
        compiled_files = {}
        main_transform = build_transform(@instructions, compiled_files:)
        source = merge_cpp_with_template(main_transform)
        outs << out_file_for_source(source:, ruby_path: @compiler_context[:source_path])
        @compiler_context[:required_ruby_files].each do |name, loaded_file|
          fn = compiled_files.fetch(name)
          transform = build_transform(loaded_file.instructions, compiled_files:)
          source = merge_cpp_with_loaded_file_template(transform, name:, fn:)
          outs << out_file_for_source(source:, ruby_path: name)
        end
        outs
      end

      # def write_file
      #   @out_file = build_main_out_file
      #   @cpp_path = @out_file.write_source_to_tempfile
      # end

      def check_build
        return if File.file?(File.join(BUILD_DIR, "libnatalie_base.#{DL_EXT}"))

        puts 'please run: rake'
        exit 1
      end

      def out_file_for_source(source:, ruby_path:)
        OutFile.new(
          source:,
          ruby_path:,
          compiler: @compiler,
          compiler_context: @compiler_context,
        )
      end

      def build_transform(instructions, compiled_files:)
        Transform.new(
          instructions,
          compiler_context: @compiler_context,
          symbols:          @symbols,
          interned_strings: @interned_strings,
          inline_functions: @inline_functions,
          compiled_files:,
        )
      end

      def merge_cpp_with_template(transform, template: nil)
        body = transform.transform('return')
        top = transform.top
        template ||= get_template
        result = template
          .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations(top:) }
          .sub('/*' + 'NAT_OBJ_INIT' + '*/') { init_object_files.join("\n") }
          .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
          .sub('/*' + 'NAT_EVAL_BODY' + '*/') { body }
        reindent(result)
      end

      def merge_cpp_with_loaded_file_template(transform, name:, fn:)
        template = LOADED_FILE_TEMPLATE
          .sub('__FN_NAME__', fn)
          .sub('"FILE_NAME"_s', "#{name.inspect}_s")
        merge_cpp_with_template(transform, template:)
      end

      def get_template
        if write_object_file?
          OBJ_TEMPLATE.gsub(/OBJ_NAME/, obj_name)
        else
          MAIN_TEMPLATE
        end
      end

      def obj_name
        @compiler
          .write_obj_source_path
          .sub(/\.rb\.cpp/, '')
          .sub(%r{.*build/(generated/)?}, '')
          .tr('/', '_')
      end

      def var_prefix
        if write_object_file?
          "#{obj_name}_"
        elsif @compiler.repl?
          "repl#{@compiler.repl_num}_"
        else
          ''
        end
      end

      def write_object_file?
        !!@compiler.write_obj_source_path
      end

      def declarations(top:)
        [
          object_file_declarations,
          symbols_declaration,
          interned_strings_declaration,
          top.join("\n")
        ].join("\n\n")
      end

      def init_matter
        [
          init_symbols.join("\n"),
          init_interned_strings.join("\n"),
          init_dollar_zero_global,
        ].compact.join("\n\n")
      end

      def object_file_declarations
        object_files.map { |name| "Value init_#{name.tr('/', '_')}(Env *env, Value self);" }.join("\n")
      end

      def symbols_declaration
        "static SymbolObject *#{symbols_var_name}[#{@symbols.size}] = {};"
      end

      def interned_strings_declaration
        return '' if @interned_strings.empty?

        "static StringObject *#{interned_strings_var_name}[#{@interned_strings.size}] = { 0 };"
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

      def init_interned_strings
        return [] if @interned_strings.empty?

        # Start with setting the interned strings list all to nullptr and register the GC hook before creating strings
        # Otherwise, we might start GC before we finished setting up this structure if the source contains enough strings
        [
          "GlobalEnv::the()->set_interned_strings(#{interned_strings_var_name}, #{@interned_strings.size});"
        ] + @interned_strings.flat_map do |(str, encoding), index|
          enum = encoding.name.tr('-', '_').upcase
          encoding_object = "EncodingObject::get(Encoding::#{enum})"
          new_string = if str.empty?
                         "#{interned_strings_var_name}[#{index}] = new StringObject(#{encoding_object});"
                       else
                         "#{interned_strings_var_name}[#{index}] = new StringObject(#{string_to_cpp(str)}, #{str.bytesize}, #{encoding_object});"
                       end
          [
            new_string,
            "#{interned_strings_var_name}[#{index}]->freeze();",
          ]
        end
      end

      def init_dollar_zero_global
        return if write_object_file?

        "env->global_set(\"$0\"_s, new StringObject { #{@compiler_context[:source_path].inspect} });"
      end

      def symbols_var_name
        static_var_name('symbols')
      end

      def interned_strings_var_name
        static_var_name('interned_strings')
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
          compile_cxx_flags: [],
          compile_ld_flags:  [],
          var_prefix:        var_prefix,
        )
      end

      def linker(paths)
        Linker.new(
          in_paths: paths,
          out_path: @compiler.out_path,
          compiler: @compiler,
          compiler_context: @compiler_context,
        )
      end

      def link(paths)
        linker(paths).link
      end
    end
  end
end
