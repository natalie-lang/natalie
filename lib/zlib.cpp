#include "zlib/zlib.h"
#include "natalie.hpp"
#include <algorithm>

using namespace Natalie;

Value init_zlib(Env *env, Value self) {
    return NilObject::the();
}

void Zlib_stream_cleanup(VoidPObject *self) {
    auto stream = (z_stream *)self->void_ptr();
    deflateEnd(stream);
    delete stream;
}

void Zlib_buffer_cleanup(VoidPObject *self) {
    auto buffer = (unsigned char *)self->void_ptr();
    delete[] buffer;
}

Value Zlib_ZStream_adler(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    return Value::integer(strm->adler);
}

Value Zlib_ZStream_avail_in(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    return Value::integer(strm->avail_in);
}

Value Zlib_ZStream_avail_out(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    return Value::integer(strm->avail_out);
}

Value Zlib_ZStream_data_type(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    return Value::integer(strm->data_type);
}

static constexpr size_t ZLIB_BUF_SIZE = 16384;

Value Zlib_deflate_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 4);
    auto Zlib = GlobalEnv::the()->Object()->const_get("Zlib"_s);
    auto level = args.at(0, Zlib->const_get("DEFAULT_COMPRESSION"_s))->as_integer_or_raise(env);
    auto window_bits = args.at(1, Zlib->const_get("MAX_WBITS"_s))->as_integer_or_raise(env);
    auto mem_level = args.at(2, Zlib->const_get("DEF_MEM_LEVEL"_s))->as_integer_or_raise(env);
    auto strategy = args.at(3, Zlib->const_get("DEFAULT_STRATEGY"_s))->as_integer_or_raise(env);

    auto stream = new z_stream {};
    self->ivar_set(env, "@stream"_s, new VoidPObject(stream, Zlib_stream_cleanup));
    self->ivar_set(env, "@result"_s, new StringObject("", Encoding::ASCII_8BIT));
    auto in = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@in"_s, new VoidPObject(in, Zlib_buffer_cleanup));
    auto out = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@out"_s, new VoidPObject(out, Zlib_buffer_cleanup));
    self->ivar_set(env, "@closed"_s, FalseObject::the());

    int ret = deflateInit2(stream,
        (int)level->to_nat_int_t(),
        Z_DEFLATED,
        (int)window_bits->to_nat_int_t(),
        (int)mem_level->to_nat_int_t(),
        (int)strategy->to_nat_int_t());
    if (ret != Z_OK)
        self->klass()->send(env, "_error"_s, { Value::integer(ret) });

    return self;
}

void Zlib_do_deflate(Env *env, Value self, const String &string, int flush) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    auto result = self->ivar_get(env, "@result"_s)->as_string_or_raise(env);
    auto in = (unsigned char *)self->ivar_get(env, "@in"_s)->as_void_p()->void_ptr();
    auto out = (unsigned char *)self->ivar_get(env, "@out"_s)->as_void_p()->void_ptr();

    size_t index = 0;
    do {
        size_t length = std::min(ZLIB_BUF_SIZE, string.length() - index);
        strm->avail_in = length;
        memcpy(in, string.c_str() + index, length);
        index += length;
        strm->next_in = in;

        do {
            strm->avail_out = ZLIB_BUF_SIZE;
            strm->next_out = out;

            int ret = deflate(strm, flush);
            if (ret == Z_STREAM_ERROR) {
                auto Error = fetch_nested_const({ "Zlib"_s, "Error"_s })->as_class_or_raise(env);
                env->raise(Error, "stream is not ready");
            }
            int have = ZLIB_BUF_SIZE - strm->avail_out;
            result->append((char *)out, have);
        } while (strm->avail_out == 0);
    } while (index < string.length());
}

Value Zlib_deflate_append(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto string = args[0]->as_string_or_raise(env);

    Zlib_do_deflate(env, self, string->string(), Z_NO_FLUSH);

    return self;
}

Value Zlib_deflate_deflate(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto string = args[0]->as_string_or_raise(env);
    auto flush = Z_NO_FLUSH;
    if (auto flush_obj = args.at(1, nullptr); flush_obj)
        flush = flush_obj->as_integer_or_raise(env)->to_nat_int_t();

    Zlib_do_deflate(env, self, string->string(), flush);

    return self->ivar_get(env, "@result"_s);
}

Value Zlib_deflate_set_dictionary(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto dictionary = args.at(0)->to_str(env);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    if (const auto ret = deflateSetDictionary(strm, reinterpret_cast<const Bytef *>(dictionary->c_str()), dictionary->bytesize()); ret != Z_OK)
        self->klass()->send(env, "_error"_s, { Value::integer(ret) });
    return self;
}

Value Zlib_deflate_params(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 2);
    auto level = args.at(0)->as_integer_or_raise(env);
    auto strategy = args.at(1)->as_integer_or_raise(env);
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();

    // Ruby supports changing the params for a stream with content, zlib does not. So instead of simply changing the
    // params, we need a few extra steps.
    //
    // 1. Finish the current stream and save that in a temporary variable
    // 2. Inflate the temporary variable
    // 3. Reset the stream and change the params
    // 4. Deflate the temporary variable to put the original stream back in

    auto original_stream = self->send(env, "finish"_s);

    auto Zlib = GlobalEnv::the()->Object()->const_get("Zlib"_s);
    auto inflated = Zlib->send(env, "inflate"_s, { original_stream });

    if (const auto ret = deflateReset(strm); ret != Z_OK)
        self->klass()->send(env, "_error"_s, { Value::integer(ret) });

    const auto ret = deflateParams(
        strm,
        (int)level->to_nat_int_t(),
        (int)strategy->to_nat_int_t());
    if (ret != Z_OK)
        self->klass()->send(env, "_error"_s, { Value::integer(ret) });

    auto result = self->ivar_get(env, "@result"_s)->as_string_or_raise(env);
    result->clear(env);

    self->send(env, "<<"_s, { inflated });

    return self;
}

Value Zlib_deflate_close(Env *env, Value self, Args args, Block *) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    deflateEnd(strm);
    self->ivar_set(env, "@closed"_s, TrueObject::the());
    return self;
}

Value Zlib_deflate_finish(Env *env, Value self, Args args, Block *) {
    Zlib_do_deflate(env, self, "", Z_FINISH);
    return self->ivar_get(env, "@result"_s);
}

Value Zlib_inflate_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto window_bits = args.at(0, fetch_nested_const({ "Zlib"_s, "MAX_WBITS"_s }))->as_integer_or_raise(env);

    auto stream = new z_stream {};
    self->ivar_set(env, "@stream"_s, new VoidPObject(stream, Zlib_stream_cleanup));
    self->ivar_set(env, "@result"_s, new StringObject("", Encoding::ASCII_8BIT));
    auto in = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@in"_s, new VoidPObject(in, Zlib_buffer_cleanup));
    auto out = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@out"_s, new VoidPObject(out, Zlib_buffer_cleanup));
    self->ivar_set(env, "@closed"_s, FalseObject::the());

    int ret = inflateInit2(stream, (int)window_bits->to_nat_int_t());
    if (ret != Z_OK)
        self->klass()->send(env, "_error"_s, { Value::integer(ret) });

    return self;
}

void Zlib_do_inflate(Env *env, Value self, const String &string, int flush) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    auto result = self->ivar_get(env, "@result"_s)->as_string_or_raise(env);
    auto in = (unsigned char *)self->ivar_get(env, "@in"_s)->as_void_p()->void_ptr();
    auto out = (unsigned char *)self->ivar_get(env, "@out"_s)->as_void_p()->void_ptr();

    int ret;

    size_t index = 0;
    do {
        size_t length = std::min(ZLIB_BUF_SIZE, string.length() - index);
        strm->avail_in = length;
        if (strm->avail_in == 0)
            break;
        memcpy(in, string.c_str() + index, length);
        strm->next_in = in;

        index += length;

        // run inflate() on input until output buffer not full
        do {
            strm->avail_out = ZLIB_BUF_SIZE;
            strm->next_out = out;
            ret = inflate(strm, Z_NO_FLUSH);

            switch (ret) {
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_NEED_DICT:
                inflateEnd(strm);
                self->klass()->send(env, "_error"_s, { Value::integer(ret) });
                break;
            case Z_STREAM_ERROR: {
                auto Error = fetch_nested_const({ "Zlib"_s, "Error"_s })->as_class_or_raise(env);
                env->raise(Error, "stream is not ready");
            }
            }

            int have = ZLIB_BUF_SIZE - strm->avail_out;
            result->append((char *)out, have);
        } while (strm->avail_out == 0);

    } while (ret != Z_STREAM_END);
}

Value Zlib_inflate_append(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto string = args[0]->as_string_or_raise(env);

    Zlib_do_inflate(env, self, string->string(), Z_NO_FLUSH);

    return self;
}

Value Zlib_inflate_inflate(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto string = args[0]->as_string_or_raise(env);
    auto flush = Z_NO_FLUSH;
    if (auto flush_obj = args.at(1, nullptr); flush_obj)
        flush = flush_obj->as_integer_or_raise(env)->to_nat_int_t();

    Zlib_do_inflate(env, self, string->string(), flush);

    return self->ivar_get(env, "@result"_s);
}

Value Zlib_inflate_finish(Env *env, Value self, Args args, Block *) {
    return self->ivar_get(env, "@result"_s);
}

Value Zlib_inflate_close(Env *env, Value self, Args args, Block *) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    inflateEnd(strm);
    self->ivar_set(env, "@closed"_s, TrueObject::the());
    return self;
}

Value Zlib_adler32(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 2);
    auto string = args.at(0, new StringObject { "", Encoding::ASCII_8BIT })->to_str(env);
    auto checksum = args.at(1, Value::integer(1))->to_int(env);
    checksum->assert_fixnum(env);
    const nat_int_t result = adler32_z(checksum->to_nat_int_t(), reinterpret_cast<const Bytef *>(string->c_str()), string->bytesize());
    return Value::integer(result);
}

Value Zlib_crc32(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 2);
    unsigned long crc;
    if (args.size() < 2) {
        crc = crc32(0L, Z_NULL, 0);
    } else {
        auto initcrcval = args.at(1);
        initcrcval->assert_type(env, Object::Type::Integer, "Integer");
        auto crc_temp = IntegerObject::convert_to_nat_int_t(env, initcrcval);
        if (crc_temp < std::numeric_limits<long>::min())
            env->raise("RangeError", "integer {} too small to convert to `long'", crc_temp);
        else if (crc_temp > std::numeric_limits<long>::max())
            env->raise("RangeError", "integer {} too big to convert to `long'", crc_temp);
        crc = (unsigned long)(crc_temp);
        // crc = IntegerObject::convert_to_ulong(env, initcrcval);
    }
    if (args.size() > 0) {
        Value string = args.at(0);
        string->assert_type(env, Object::Type::String, "String");
        crc = ::crc32(crc, (Bytef *)(string->as_string()->c_str()), string->as_string()->string().size());
    }
    return new IntegerObject { (nat_int_t)crc };
}

Value Zlib_crc_table(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto res = new ArrayObject { 256 };
    auto table = get_crc_table();
    for (size_t i = 0; i < 256; i++)
        res->push(Value::integer(static_cast<nat_int_t>(table[i])));
    return res;
}

Value Zlib_zlib_version(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    return new StringObject { ZLIB_VERSION, Encoding::ASCII_8BIT };
}
