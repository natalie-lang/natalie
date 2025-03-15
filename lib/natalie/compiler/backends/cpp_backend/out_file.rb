require 'tempfile'
require_relative '../../flags'
require_relative './pkg_config'
require_relative '../../string_to_cpp'

module Natalie
  class Compiler
    class CppBackend
      class OutFile
        include Flags
        include PkgConfig
        include StringToCpp

        ROOT_DIR = File.expand_path('../../../../../', __dir__)
        SRC_PATH = File.join(ROOT_DIR, 'src')
        MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main_template.cpp'))
        OBJ_TEMPLATE = File.read(File.join(SRC_PATH, 'obj_unit_template.cpp'))
        LOADED_FILE_TEMPLATE = File.read(File.join(SRC_PATH, 'loaded_file_template.cpp'))

        def initialize(type:, body:, transform_data:, ruby_path:, compiler:, backend:)
          @type = type
          raise "type must be :main, :obj, or :loaded_file" unless %i[main obj loaded_file].include?(type)
          @body = body
          @top = transform_data.top
          @symbols = transform_data.symbols
          @interned_strings = transform_data.interned_strings
          @var_prefix = transform_data.var_prefix
          @ruby_path = ruby_path
          @compiler = compiler
          @backend = backend
        end

        attr_reader :ruby_path,
                    :cpp_path,
                    :status

        def write_source
          if build_dir
            @cpp_path = build_path('cpp')
            FileUtils.mkdir_p(File.dirname(@cpp_path))
            write_source_to_path(@cpp_path)
          else
            write_source_to_tempfile
          end
        end

        def write_source_to_tempfile
          temp = Tempfile.create(temp_name('cpp'))
          temp.write(merged_source)
          temp.close
          @cpp_path = temp.path
        end

        def write_source_to_path(path)
          return if File.exist?(path) && File.read(path) == merged_source

          File.write(path, merged_source)
        end

        def append_loaded_file(other_out_file)
          raise "Expected OutFile, got #{other_out_file.class}" unless other_out_file.is_a?(OutFile)

          fn = @backend.compiled_files.fetch(other_out_file.ruby_path)
          @top["#{fn}_def"] = other_out_file.loaded_file_fn_source(fn_only: true)
        end

        def compile_object_file
          write_source unless @cpp_path
          if build_dir && File.exist?(out_path) && File.stat(out_path).mtime >= File.stat(@cpp_path).mtime
            @status = :unchanged
            return out_path
          end

          cmd = compiler_command
          puts cmd if @compiler.debug == 'cc-cmd'
          out = `#{cmd} 2>&1`
          File.unlink(@cpp_path) unless @compiler.keep_cpp? || build_dir || $? != 0
          puts "cpp file path is: #{@cpp_path}" if @compiler.keep_cpp?
          warn out if out.strip != ''
          raise Compiler::CompileError, 'There was an error compiling.' if $? != 0

          @status = :compiled
          out_path
        end

        def compiler_command
          [
            cc,
            build_flags,
            (@compiler.repl? ? LIBNAT_AND_REPL_FLAGS.join(' ') : ''),
            INC_PATHS.map { |path| "-I #{path}" }.join(' '),
            *package_compile_flags,
            '-c',
            "-o #{out_path}",
            '-x c++ -std=c++17',
            (@cpp_path || 'code.cpp'),
          ].map(&:to_s).join(' ')
        end

        def out_path
          @out_path ||= if build_dir
            build_path('o')
          else
            out = Tempfile.create(temp_name('o'))
            out.close
            out.path
          end
        end

        def relative_ruby_path
          if File.absolute_path?(@ruby_path) && @ruby_path.start_with?(Dir.pwd)
            @ruby_path[(Dir.pwd.size + 1)..-1]
          else
            @ruby_path
          end
        end

        def merged_source
          @mereged_source ||= begin
            result = get_template
              .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
              .sub('/*' + 'NAT_OBJ_INIT' + '*/') { init_object_files }
              .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
              .sub('/*' + 'NAT_EVAL_BODY' + '*/') { @body }
            reindent(result)
          end
        end

        def loaded_file_fn_source(fn_only: false)
          fn = @backend.compiled_files.fetch(@ruby_path)
          source = LOADED_FILE_TEMPLATE
          source = source.sub(/.*(?=Value __FN_NAME__)/m, '') if fn_only
          result = source
            .sub('__FN_NAME__', fn)
            .sub('"FILE_NAME"_s', "#{@ruby_path.inspect}_s")
            .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
            .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
            .sub('/*' + 'NAT_EVAL_BODY' + '*/') { @body }
          reindent(result)
        end

        private

        def get_template
          case @type
          when :main
            MAIN_TEMPLATE
          when :obj
            OBJ_TEMPLATE.gsub(/OBJ_NAME/, @backend.obj_name)
          when :loaded_file
            loaded_file_fn_source
          else
            raise "Unexpected type: #{@type.inspect}"
          end
        end

        def cc
          ENV['CXX'] || 'c++'
        end

        def build_flags
          (
            base_build_flags +
            [ENV['NAT_CXX_FLAGS']].compact +
            @backend.compiler_context[:compile_cxx_flags]
          ).join(' ')
        end

        def base_build_flags
          case @compiler.build
          when 'release'
            RELEASE_FLAGS
          when 'debug', nil
            DEBUG_FLAGS
          when 'sanitized'
            SANITIZED_FLAGS
          when 'coverage'
            COVERAGE_FLAGS
          else
            raise "unknown build mode: #{@compiler.build.inspect}"
          end
        end

        def build_dir = @compiler.build_dir
        def single_source? = !build_dir

        def build_path(extension)
          File.join(
            build_dir,
            relative_ruby_path
              .sub(/\.rb$/, ".#{extension}")
              .gsub(/[^a-zA-Z0-9_\.\/]/, '_')
          )
        end

        def temp_name(extension)
          File.split(@ruby_path)
            .last
            .sub(/(\.rb)?$/, ".#{extension}")
            .gsub(/[^a-zA-Z0-9_\.\/]/, '_')
        end

        def declarations
          [
            object_file_declarations,
            symbols_declaration,
            interned_strings_declaration,
            @top.values.join("\n")
          ].join("\n\n")
        end

        def init_matter
          return '' if single_source? && @type == :loaded_file

          [
            init_symbols.join("\n"),
            init_interned_strings.join("\n"),
            init_dollar_zero_global,
          ].compact.join("\n\n")
        end

        def object_file_declarations
          return '' if @type == :loaded_file

          object_files.map do |name|
            "Value init_#{name.tr('/', '_')}(Env *env, Value self);"
          end.join("\n")
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
          end.join("\n")
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
          return unless @type == :main

          "env->global_set(\"$0\"_s, new StringObject { #{@backend.compiler_context[:source_path].inspect} });"
        end

        def symbols_var_name
          static_var_name('symbols')
        end

        def interned_strings_var_name
          static_var_name('interned_strings')
        end

        def static_var_name(suffix)
          "#{@var_prefix}#{suffix}"
        end

        def object_files
          rb_files = Dir[File.join(ROOT_DIR, '/src/**/*.rb')].map do |path|
            path.sub(%r{^.*/src/}, '')
          end.grep(%r{^([a-z0-9_]+/)?[a-z0-9_]+\.rb$})
          list = rb_files.sort.map { |name| name.split('.').first }
          ['exception'] + # must come first
          (list - ['exception']) +
          @backend.compiler_context[:required_cpp_files].values
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
end
