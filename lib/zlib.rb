require 'natalie/inline'

__inline__ '#include "zlib/zlib.h"'
__inline__ '#include <algorithm>'
__ld_flags__ '-lz'

module Zlib
  DEFAULT_COMPRESSION = -1
  NO_COMPRESSION = 0
  BEST_SPEED = 1
  BEST_COMPRESSION = 9

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

  class Deflate
    class << self
      __define_method__ :deflate, <<-END
        args.ensure_argc_between(env, 1, 2);
        auto value = args[0];
        auto level = args.size() > 1 ? args[1] : Value::integer(-1);

        const size_t CHUNK = 16384;
        int ret, flush;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        int level_num = -1;

        auto string = value->as_string_or_raise(env);
        level_num = (int)level->as_integer_or_raise(env)->to_nat_int_t();

        auto result = new StringObject;

        ret = deflateInit(&strm, level_num);
        if (ret != Z_OK)
            self->send(env, "_error"_s, { Value::integer(ret) });

        size_t index = 0;
        do {
            size_t length = std::min(CHUNK, string->bytesize() - index);
            strm.avail_in = length;
            memcpy(in, string->string().c_str() + index, length);
            index += length;
            flush = index >= string->bytesize() ? Z_FINISH : Z_NO_FLUSH;
            strm.next_in = in;

            // run deflate() on input until output buffer not full,
            // finish compression if all of source has been read in
            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = deflate(&strm, flush);
                assert(ret != Z_STREAM_ERROR);
                have = CHUNK - strm.avail_out;
                result->append((char*)out, have);
            } while (strm.avail_out == 0);
            assert(strm.avail_in == 0);
        } while (flush != Z_FINISH);

        deflateEnd(&strm);

        assert(ret == Z_STREAM_END);

        return result;
      END

      def _error(num)
        Zlib._error(num)
      end
    end
  end

  class Inflate
    class << self
      __define_method__ :inflate, [:value], <<-END
        const size_t CHUNK = 16384;

        int ret;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];

        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;

        auto string = value->as_string_or_raise(env);

        auto result = new StringObject;

        ret = inflateInit(&strm);
        if (ret != Z_OK)
            self->send(env, "_error"_s, { Value::integer(ret) });

        size_t index = 0;
        do {
            size_t length = std::min(CHUNK, string->bytesize() - index);
            strm.avail_in = length;
            if (strm.avail_in == 0)
                break;
            memcpy(in, string->string().c_str() + index, length);
            strm.next_in = in;

            index += length;

            // run inflate() on input until output buffer not full
            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = inflate(&strm, Z_NO_FLUSH);
                assert(ret != Z_STREAM_ERROR);

                switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    inflateEnd(&strm);
                    self->send(env, "_error"_s, { Value::integer(ret) });
                    break;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&strm);
                    self->send(env, "_error"_s, { Value::integer(ret) });
                    break;
                }

                have = CHUNK - strm.avail_out;
                result->append((char*)out, have);
            } while (strm.avail_out == 0);

        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);
        if (ret != Z_STREAM_END)
            self->send(env, "_error"_s, { Value::integer(Z_DATA_ERROR) });

        return result;
      END

      def _error(num)
        Zlib._error(num)
      end
    end
  end
end
