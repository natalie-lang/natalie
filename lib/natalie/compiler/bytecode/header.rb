module Natalie
  class Compiler
    module Bytecode
      class Header
        # Current version is 0.0, which means we do not guarantee any backwards compatibility
        MAJOR_VERSION = 0
        MINOR_VERSION = 0

        MAGIC_HEADER = 'NatX'

        attr_reader :magic_header, :major_version, :minor_version

        def initialize
          @magic_header = MAGIC_HEADER
          @major_version = MAJOR_VERSION
          @minor_version = MINOR_VERSION
        end

        def self.load(io)
          magic_header, major_version, minor_version = io.read(6).unpack('a4C2')
          raise 'Invalid header, this is probably not a Natalie bytecode file' if magic_header != MAGIC_HEADER
          if major_version != MAJOR_VERSION || minor_version != MINOR_VERSION
            raise "Invalid version, expected #{MAJOR_VERSION}.#{MINOR_VERSION}, got #{major_version}.#{minor_version}"
          end

          result = new
          result.send(:magic_header=, magic_header)
          result.send(:major_version=, major_version)
          result.send(:minor_version=, minor_version)
          result
        end

        # Format: Magic header (32 bits), major version (8 bits), minor version (8 bits)
        def to_s
          @header ||= [magic_header, major_version, minor_version].pack('a4C2')
        end

        def bytesize = 6

        private

        attr_writer :magic_header, :major_version, :minor_version
      end
    end
  end
end
