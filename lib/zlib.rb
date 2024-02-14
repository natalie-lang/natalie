require 'natalie/inline'
require 'zlib.cpp'

__ld_flags__ '-lz'

module Zlib
  class Error < StandardError; end
  class BufError < Error; end
  class DataError < Error; end
  class MemError < Error; end
  class NeedDict < Error; end
  class StreamEnd < Error; end
  class StreamError < Error; end
  class VersionError < Error; end

  def self._error(num)
    case num
    when 1
      raise StreamEnd
    when 2
      raise NeedDict
    when -1
      __inline__ "env->raise_errno();"
    when -2
      raise StreamError
    when -3
      raise DataError
    when -4
      raise MemError
    when -5
      raise BufError
    when -6
      raise VersionError
    else
      raise Error, "unknown error"
    end
  end

  #class << self
  __bind_static_method__ :adler32, :Zlib_adler32
  __bind_static_method__ :crc32, :Zlib_crc32
  __bind_static_method__ :crc_table, :Zlib_crc_table
  __bind_static_method__ :zlib_version, :Zlib_zlib_version
  #end

  def self.deflate(...)
    Deflate.deflate(...)
  end

  def self.inflate(...)
    Inflate.inflate(...)
  end

  class ZStream
  end
  
  class Deflate < ZStream
    __bind_method__ :initialize, :Zlib_deflate_initialize
    __bind_method__ :<<, :Zlib_deflate_append
    __bind_method__ :set_dictionary, :Zlib_deflate_set_dictionary
    __bind_method__ :finish, :Zlib_deflate_finish
    __bind_method__ :close, :Zlib_deflate_close

    class << self
      def deflate(input, level = -1)
        zstream = Zlib::Deflate.new(level)
        zstream << input
        zstream.finish.tap do
          zstream.close                                                                                                                                                                           
        end
      end

      def _error(num)
        Zlib._error(num)
      end
    end
  end

  class Inflate < ZStream
    __bind_method__ :initialize, :Zlib_inflate_initialize
    __bind_method__ :<<, :Zlib_inflate_append
    __bind_method__ :finish, :Zlib_inflate_finish
    __bind_method__ :close, :Zlib_inflate_close

    class << self
      def inflate(input)
        zstream = Zlib::Inflate.new                                                                                                                                                             
        zstream << input
        zstream.finish.tap do
          zstream.close
        end
      end

      def _error(num)
        Zlib._error(num)
      end
    end
  end
end
