require 'tempfile'
require_relative '../../flags'

module Natalie
  class Compiler
    class CppBackend
      class OutFile
        include Flags

        def initialize(source:, compiler:, compiler_context:)
          @source = source
          @compiler = compiler
          @compiler_context = compiler_context
        end

        attr_reader :source_path # {tempfile}.cpp

        def write_source_to_tempfile
          temp = Tempfile.create('natalie.cpp')
          temp.write(@source)
          temp.close
          @source_path = temp.path
        end

        def write_source_to_path(path)
          File.write(path, @source)
          @source_path = path
        end

        def compile_object_file
          raise 'no source file yet' unless @source_path
          cmd = compiler_command
          out = `#{cmd} 2>&1`
          File.unlink(@source_path) unless @compiler.keep_cpp? || $? != 0
          puts "cpp file path is: #{@source_path}" if @compiler.keep_cpp?
          warn out if out.strip != ''
          raise Compiler::CompileError, 'There was an error compiling.' if $? != 0
        end

        def compiler_command
          [
            cc,
            build_flags,
            (@compiler.repl? ? LIBNAT_AND_REPL_FLAGS.join(' ') : ''),
            inc_paths.map { |path| "-I #{path}" }.join(' '),
            "-o #{@compiler.out_path}",
            '-x c++ -std=c++17',
            (@source_path || 'code.cpp'),
            lib_paths.map { |path| "-L #{path}" }.join(' '),
            libraries.join(' '),
            link_flags,
          ].map(&:to_s).join(' ')
        end

        private

        def cc
          ENV['CXX'] || 'c++'
        end

        def inc_paths
          INC_PATHS +
          PACKAGES_REQUIRING_PKG_CONFIG.flat_map do |package|
            flags_for_package(package, :inc)
          end.compact
        end

        def lib_paths
          LIB_PATHS +
          PACKAGES_REQUIRING_PKG_CONFIG.flat_map do |package|
            flags_for_package(package, :lib)
          end.compact
        end

        # FIXME: We should run this on any system (not just Darwin), but only when one
        # of the packages in PACKAGES_REQUIRING_PKG_CONFIG are used.
        def flags_for_package(package, type)
          return unless DARWIN

          @flags_for_package ||= {}
          existing_flags = @flags_for_package[package]
          return existing_flags[type] if existing_flags

          unless system("pkg-config --exists #{package}")
            @flags_for_package[package] = { inc: [], lib: [] }
            return []
          end

          flags = @flags_for_package[package] = {}
          unless (inc_result = `pkg-config --cflags #{package}`.strip).empty?
            flags[:inc] = inc_result.sub(/^-I/, '')
          end
          unless (lib_result = `pkg-config --libs-only-L #{package}`.strip).empty?
            flags[:lib] = lib_result.sub(/^-L/, '')
          end

          flags[type]
        end

        def link_flags
          flags = if @compiler.build == 'sanitized'
                    [SANITIZE_FLAG]
                  else
                    []
                  end
          flags += @compiler_context[:compile_ld_flags].join(' ').split
          flags -= unnecessary_link_flags
          flags.join(' ')
        end

        def libraries
          if @compiler.repl?
            []
          elsif @compiler.dynamic_linking?
            LIBRARIES_FOR_DYNAMIC_LINKING
          else
            LIBRARIES_FOR_STATIC_LINKING
          end
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

        def unnecessary_link_flags
          OPENBSD ? ['-ldl'] : []
        end
      end
    end
  end
end
