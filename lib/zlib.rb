require 'natalie/inline'
require 'zlib.cpp'

__ld_flags__ '-lz'

module Zlib
  __constant__ 'ASCII', 'int', 'Z_ASCII'
  __constant__ 'BEST_COMPRESSION', 'int', 'Z_BEST_COMPRESSION'
  __constant__ 'BEST_SPEED', 'int', 'Z_BEST_SPEED'
  __constant__ 'BINARY', 'int', 'Z_BINARY'
  __constant__ 'DEFAULT_COMPRESSION', 'int', 'Z_DEFAULT_COMPRESSION'
  __constant__ 'DEFAULT_STRATEGY', 'int', 'Z_DEFAULT_STRATEGY'
  DEF_MEM_LEVEL = 8 # Not defined in the zlib source, value copied from MRI documentation
  __constant__ 'FILTERED', 'int', 'Z_FILTERED'
  __constant__ 'FINISH', 'int', 'Z_FINISH'
  __constant__ 'FIXED', 'int', 'Z_FIXED'
  __constant__ 'HUFFMAN_ONLY', 'int', 'Z_HUFFMAN_ONLY'
  __constant__ 'MAX_MEM_LEVEL', 'int', 'MAX_MEM_LEVEL'
  __constant__ 'MAX_WBITS', 'int', 'MAX_WBITS'
  __constant__ 'NO_COMPRESSION', 'int', 'Z_NO_COMPRESSION'
  __constant__ 'RLE', 'int', 'Z_RLE'
  __constant__ 'UNKNOWN', 'int', 'Z_UNKNOWN'

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
  __bind_static_method__ :crc_table, :Zlib_crc_table, 0
  __bind_static_method__ :zlib_version, :Zlib_zlib_version, 0
  #end

  def self.deflate(...)
    Deflate.deflate(...)
  end

  def self.inflate(...)
    Inflate.inflate(...)
  end

  class ZStream
    __bind_method__ :adler, :Zlib_ZStream_adler, 0
    __bind_method__ :avail_in, :Zlib_ZStream_avail_in, 0
    __bind_method__ :avail_out, :Zlib_ZStream_avail_out, 0
    __bind_method__ :data_type, :Zlib_ZStream_data_type, 0

    def closed? = @closed
  end
  
  class Deflate < ZStream
    __bind_method__ :initialize, :Zlib_deflate_initialize
    __bind_method__ :<<, :Zlib_deflate_append, 1
    __bind_method__ :deflate, :Zlib_deflate_deflate
    __bind_method__ :set_dictionary, :Zlib_deflate_set_dictionary, 1
    __bind_method__ :params, :Zlib_deflate_params, 2
    __bind_method__ :finish, :Zlib_deflate_finish, 0
    __bind_method__ :close, :Zlib_deflate_close, 0

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
    __bind_method__ :<<, :Zlib_inflate_append, 1
    __bind_method__ :inflate, :Zlib_inflate_inflate
    __bind_method__ :finish, :Zlib_inflate_finish, 0
    __bind_method__ :close, :Zlib_inflate_close, 0

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

  class GzipFile
    class Error < Zlib::Error; end

    attr_writer :comment, :orig_name

    def initialize(io)
      @io = io
      @closed = false
    end

    def close
      @closed = true
    end

    def closed?
      @closed
    end

    def comment
      raise GzipFile::Error, 'closed gzip stream' if @closed

      @comment
    end

    def orig_name
      raise GzipFile::Error, 'closed gzip stream' if @closed

      @orig_name
    end
  end

  class GzipWriter < GzipFile
    class << self
      def wrap(io)
        gzip_writer = new(io)
        yield(gzip_writer)
      ensure
        gzip_writer.close
      end
    end
  end
end
