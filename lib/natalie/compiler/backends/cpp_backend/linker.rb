require 'tempfile'
require_relative '../../flags'
require_relative './pkg_config'

module Natalie
  class Compiler
    class CppBackend
      class Linker
        include PkgConfig
        include Flags

        def initialize(in_paths:, out_path:, compiler:, compiler_context:)
          @in_paths = in_paths
          @out_path = out_path
          @compiler = compiler
          @compiler_context = compiler_context
        end

        def link
          cmd = link_command
          puts cmd if @compiler.debug == 'cc-cmd'
          out = `#{cmd} 2>&1`
          @in_paths.each { |path| File.unlink(path) } unless @compiler.build_dir || $? != 0
          warn out if out.strip != ''
          raise Compiler::CompileError, 'There was an error linking.' if $? != 0
        end

        def link_command
          [
            cc,
            LIB_PATHS.map { |path| "-L #{path}" }.join(' '),
            (@compiler.repl? ? LIBNAT_AND_REPL_LINK_FLAGS.join(' ') : ''),
            *package_link_flags,
            *@in_paths,
            libraries.join(' '),
            link_flags,
            "-o #{@compiler.out_path}",
          ].join(' ')
        end

        private

        def cc
          ENV['CXX'] || 'c++'
        end

        def link_flags
          flags = []
          flags << SANITIZE_FLAG if @compiler.build == 'sanitized'
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

        def unnecessary_link_flags
          OPENBSD ? ['-ldl'] : []
        end
      end
    end
  end
end
