module Natalie
  class Compiler
    # Platform settings and native linking shared by all code generators.
    module NativeBuild
      ROOT_DIR = File.expand_path('../../..', __dir__)
      BUILD_DIR = File.join(ROOT_DIR, 'build')

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
      LIBRARIES_FOR_DYNAMIC_LINKING = %w[-lnatalie_base -lonigmo].freeze

      # When compiling a binary with the `-c` option, we use static linking for compatibility.
      LIBRARIES_FOR_STATIC_LINKING = %w[-lnatalie].freeze

      LIB_PATHS = [BUILD_DIR, File.join(BUILD_DIR, 'onigmo/lib'), File.join(BUILD_DIR, 'zlib')].freeze

      def self.object_name(path)
        path.sub(/\.rb\.o$/, '').sub(/\.(so|dylib)$/, '').sub(%r{.*build/(generated/)?}, '').gsub(/[^a-zA-Z0-9_]/, '_')
      end
    end
  end
end

require_relative 'native_build/linker'
