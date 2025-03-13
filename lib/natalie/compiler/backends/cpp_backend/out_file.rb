require 'tempfile'
require_relative '../../flags'
require_relative './pkg_config'

module Natalie
  class Compiler
    class CppBackend
      class OutFile
        include Flags
        include PkgConfig

        def initialize(source:, ruby_path:, compiler:, compiler_context:)
          @source = source
          @ruby_path = ruby_path
          @compiler = compiler
          @compiler_context = compiler_context
        end

        attr_reader :ruby_path,
                    :cpp_path

        def write_source_to_tempfile
          temp = Tempfile.create(temp_name('cpp'))
          temp.write(@source)
          temp.close
          @cpp_path = temp.path
        end

        def write_source_to_path(path)
          File.write(path, @source)
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

        def cc
          ENV['CXX'] || 'c++'
        end

        def build_flags
          (
            base_build_flags +
            [ENV['NAT_CXX_FLAGS']].compact +
            @compiler_context[:compile_cxx_flags]
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
      end
    end
  end
end
