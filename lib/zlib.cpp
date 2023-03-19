#include "zlib/zlib.h"
#include "natalie.hpp"
#include <algorithm>

using namespace Natalie;

Value init(Env *env, Value self) {
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

static constexpr size_t ZLIB_BUF_SIZE = 16384;

Value Zlib_deflate_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto level = args.size() > 0 ? args[0]->as_integer_or_raise(env) : Value::integer(-1);

    auto stream = new z_stream {};
    self->ivar_set(env, "@stream"_s, new VoidPObject(stream, Zlib_stream_cleanup));
    self->ivar_set(env, "@result"_s, new StringObject);
    auto in = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@in"_s, new VoidPObject(in, Zlib_buffer_cleanup));
    auto out = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@out"_s, new VoidPObject(out, Zlib_buffer_cleanup));

    int ret = deflateInit(stream, (int)level->as_integer()->to_nat_int_t());
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
        assert(strm->avail_in == 0);
    } while (index < string.length());
}

Value Zlib_deflate_append(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto string = args[0]->as_string_or_raise(env);

    Zlib_do_deflate(env, self, string->string(), Z_NO_FLUSH);

    return string;
}

Value Zlib_deflate_close(Env *env, Value self, Args args, Block *) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    deflateEnd(strm);
    return self;
}

Value Zlib_deflate_finish(Env *env, Value self, Args args, Block *) {
    Zlib_do_deflate(env, self, "", Z_FINISH);
    return self->ivar_get(env, "@result"_s);
}

Value Zlib_inflate_initialize(Env *env, Value self, Args args, Block *) {
    auto stream = new z_stream {};
    self->ivar_set(env, "@stream"_s, new VoidPObject(stream, Zlib_stream_cleanup));
    self->ivar_set(env, "@result"_s, new StringObject);
    auto in = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@in"_s, new VoidPObject(in, Zlib_buffer_cleanup));
    auto out = new unsigned char[ZLIB_BUF_SIZE];
    self->ivar_set(env, "@out"_s, new VoidPObject(out, Zlib_buffer_cleanup));

    int ret = inflateInit(stream);
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
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
                inflateEnd(strm);
                self->send(env, "_error"_s, { Value::integer(ret) });
                break;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(strm);
                self->send(env, "_error"_s, { Value::integer(ret) });
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

    return string;
}

Value Zlib_inflate_finish(Env *env, Value self, Args args, Block *) {
    return self->ivar_get(env, "@result"_s);
}

Value Zlib_inflate_close(Env *env, Value self, Args args, Block *) {
    auto *strm = (z_stream *)self->ivar_get(env, "@stream"_s)->as_void_p()->void_ptr();
    inflateEnd(strm);
    return self;
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
