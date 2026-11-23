require 'digest'

module Natalie
  class Compiler
    class LlvmBackend
      # File entrypoints and return tokens cross object boundaries. Everything
      # else, including closures and literal caches, remains private to a unit.
      class Units
        attr_reader :sources

        def initialize(required_files:, native_extensions:)
          @required_files = required_files
          @native_extensions = native_extensions
          @sources = {}
        end

        def self.id(path)
          Digest::SHA256.hexdigest(path == '-e' ? path : File.expand_path(path))
        end

        def entrypoint(path)
          "nat_llvm_file_#{self.class.id(path)}"
        end

        def return_token(path)
          "nat_llvm_file_return_#{self.class.id(path)}"
        end

        def load(path)
          return if @sources.key?(path)

          @sources[path] = nil # Cyclic requires refer to the entrypoint already being built.
          @sources[path] = IR.new(
            @required_files.fetch(path).instructions,
            source_path: path,
            function_name: entrypoint(path),
            module_entry: true,
            required_files: @required_files,
            native_extensions: @native_extensions,
            units: self,
          ).generate
        end
      end
    end
  end
end
