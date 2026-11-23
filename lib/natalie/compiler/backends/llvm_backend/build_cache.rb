require 'digest'
require 'json'
require 'fileutils'
require 'shellwords'
require 'tempfile'

module Natalie
  class Compiler
    class LlvmBackend
      class BuildCache
        COMPILER_ENV = %w[
          CPATH
          CPLUS_INCLUDE_PATH
          C_INCLUDE_PATH
          SDKROOT
          MACOSX_DEPLOYMENT_TARGET
          NIX_CFLAGS_COMPILE
          NIX_LDFLAGS
        ].freeze

        def initialize(compiler, source_path)
          @compiler = compiler
          @directory =
            File.join(File.expand_path(compiler.build_dir), 'llvm', Units.id(source_path), compiler.compilation_type)
          FileUtils.mkdir_p(@directory)
        end

        def lock
          # Keep the lock inode: unlinking it could let a third compiler bypass
          # a second compiler already waiting on the original inode.
          File.open(File.join(@directory, 'build.lock'), File::RDWR | File::CREAT) do |file|
            file.flock(File::LOCK_EX)
            yield
          ensure
            file.flock(File::LOCK_UN)
          end
        end

        def llvm_object(name, source, native)
          path = File.join(@directory, "#{name}.o")
          write_if_changed(File.join(@directory, "#{name}.ll"), source)
          key = digest([source, native.cache_key, @compiler.build])
          cached(path, key) { |temporary| native.emit_object(source, temporary) }
          path
        end

        def native_object(out)
          path = File.join(@directory, 'native.o')
          source_path = File.join(@directory, 'native.cpp')
          write_if_changed(source_path, out.merged_source)
          out.cpp_path = source_path
          out.out_path = path
          command = out.compiler_command
          key = digest([out.merged_source, command, compiler_identity, COMPILER_ENV.map { |name| [name, ENV[name]] }])
          dependencies = File.join(@directory, 'native.d')
          cached(path, key) do |temporary|
            out.out_path = temporary
            out.dependency_file = "#{temporary}.d"
            begin
              out.compile_object_file(force: true)
              File.rename(out.dependency_file, dependencies)
              dependency_paths = Shellwords.split(File.read(dependencies).gsub("\\\n", ' ').gsub('$$', '$')).drop(1)
              dependency_paths.uniq.to_h { |file| [file, fingerprint(file)] }
            ensure
              File.unlink(out.dependency_file) if File.exist?(out.dependency_file)
            end
          end
          path
        end

        private

        def cached(path, key)
          manifest_path = "#{path}.json"
          manifest =
            begin
              JSON.parse(File.read(manifest_path)) if File.file?(manifest_path)
            rescue JSON::ParserError
              nil
            end
          if File.file?(path) && File.size?(path) && manifest.is_a?(Hash) && manifest['key'] == key &&
               manifest.fetch('dependencies', {}).all? { |file, signature| fingerprint(file) == signature }
            puts "unchanged: #{path}" if ENV['NAT_VERBOSE'] && !@compiler.build_quietly?
            return
          end
          Tempfile.create(%w[object- .o], @directory) do |file|
            file.close
            dependencies = yield(file.path)
            raise CompileError, 'Compiler did not produce a nonempty object' unless File.size?(file.path)
            File.rename(file.path, path)
            write_if_changed(
              manifest_path,
              JSON.generate({ key: key, dependencies: dependencies.is_a?(Hash) ? dependencies : {} }),
            )
          end
          puts "compiled: #{path}" unless @compiler.build_quietly?
        end

        def write_if_changed(path, content)
          return if File.file?(path) && File.binread(path) == content

          Tempfile.create(%w[source- .tmp], @directory) do |file|
            file.binmode
            file.write(content)
            file.close
            File.rename(file.path, path)
          end
        end

        def digest(value)
          Digest::SHA256.hexdigest(JSON.generate(value))
        end

        def fingerprint(path)
          File.file?(path.to_s) ? Digest::SHA256.file(path).hexdigest : nil
        end

        def compiler_identity
          Shellwords
            .split(ENV.fetch('CXX', 'c++'))
            .filter_map do |word|
              path =
                if word.include?(File::SEPARATOR)
                  word
                else
                  ENV
                    .fetch('PATH')
                    .split(File::PATH_SEPARATOR)
                    .map { |dir| File.join(dir, word) }
                    .find { |file| File.executable?(file) }
                end
              [File.realpath(path), fingerprint(path)] if path && File.file?(path)
            end
        end
      end
    end
  end
end
