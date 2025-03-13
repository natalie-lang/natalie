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

        def initialize(type:, body:, top:, ruby_path:, compiler:, backend:)
          @type = type
          raise "type must be :main, :obj, or :loaded_file" unless %i[main obj loaded_file].include?(type)
          @body = body
          @top = top
          @ruby_path = ruby_path
          @compiler = compiler
          @backend = backend
        end

        attr_reader :ruby_path,
                    :cpp_path

        def write_source_to_tempfile
          temp = Tempfile.create(temp_name('cpp'))
          temp.write(merged_source)
          temp.close
          @cpp_path = temp.path
        end

        def write_source_to_path(path)
          File.write(path, merged_source)
          @cpp_path = path
        end

        def compile_object_file
          write_source_to_tempfile unless @cpp_path
          cmd = compiler_command
          out = `#{cmd} 2>&1`
          File.unlink(@cpp_path) unless @compiler.keep_cpp? || $? != 0
          puts "cpp file path is: #{@cpp_path}" if @compiler.keep_cpp?
          warn out if out.strip != ''
          raise Compiler::CompileError, 'There was an error compiling.' if $? != 0
          @out_path
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
          @out_path ||= begin
            out = Tempfile.create(temp_name('o'))
            out.close
            out.path
          end
        end

        private

        def merged_source
          @merged_source ||= begin
            template ||= get_template
            result = template
              .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
              .sub('/*' + 'NAT_OBJ_INIT' + '*/') { init_object_files.join("\n") }
              .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
              .sub('/*' + 'NAT_EVAL_BODY' + '*/') { @body }
            reindent(result)
          end
        end

        def get_template
          case @type
          when :main
            MAIN_TEMPLATE
          when :obj
            OBJ_TEMPLATE.gsub(/OBJ_NAME/, @backend.obj_name)
          when :loaded_file
            fn = @backend.compiled_files.fetch(@ruby_path)
            LOADED_FILE_TEMPLATE
              .sub('__FN_NAME__', fn)
              .sub('"FILE_NAME"_s', "#{@ruby_path.inspect}_s")
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

        def temp_name(extension)
          File.split(@ruby_path).last.sub(/(\.rb)?$/, ".#{extension}")
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
          "static SymbolObject *#{symbols_var_name}[#{@backend.symbols.size}] = {};"
        end

        def interned_strings_declaration
          return '' if @backend.interned_strings.empty?

          "static StringObject *#{interned_strings_var_name}[#{@backend.interned_strings.size}] = { 0 };"
        end

        def init_object_files
          object_files.map do |name|
            "init_#{name.tr('/', '_')}(env, self);"
          end
        end

        def init_symbols
          @backend.symbols.map do |name, index|
            "#{symbols_var_name}[#{index}] = SymbolObject::intern(#{string_to_cpp(name.to_s)}, #{name.to_s.bytesize});"
          end
        end

        def init_interned_strings
          return [] if @backend.interned_strings.empty?

          # Start with setting the interned strings list all to nullptr and register the GC hook before creating strings
          # Otherwise, we might start GC before we finished setting up this structure if the source contains enough strings
          [
            "GlobalEnv::the()->set_interned_strings(#{interned_strings_var_name}, #{@backend.interned_strings.size});"
          ] + @backend.interned_strings.flat_map do |(str, encoding), index|
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
          return if @type == :obj

          "env->global_set(\"$0\"_s, new StringObject { #{@backend.compiler_context[:source_path].inspect} });"
        end

        def symbols_var_name
          static_var_name('symbols')
        end

        def interned_strings_var_name
          static_var_name('interned_strings')
        end

        def static_var_name(suffix)
          "#{@backend.compiler_context[:var_prefix]}#{suffix}"
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
