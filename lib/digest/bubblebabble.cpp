/************************************************

  bubblebabble.c - BubbleBabble encoding support

  $Author$
  created at: Fri Oct 13 18:31:42 JST 2006

  Copyright (C) 2006 Akinori MUSHA

  $Id$

************************************************/

#include <natalie/ruby/ruby.h>

static VALUE
bubblebabble_str_new(Env *env, VALUE str_digest)
{
    char *digest;
    size_t digest_len;
    VALUE str;
    char *p;
    size_t i, j, seed = 1;
    static const char vowels[] = {
        'a', 'e', 'i', 'o', 'u', 'y'
    };
    static const char consonants[] = {
        'b', 'c', 'd', 'f', 'g', 'h', 'k', 'l', 'm', 'n',
        'p', 'r', 's', 't', 'v', 'z', 'x'
    };

    StringValue(str_digest);
    digest = RSTRING_PTR(str_digest);
    digest_len = RSTRING_LEN(str_digest);

    if ((LONG_MAX - 2) / 3 < (digest_len | 1)) {
        rb_raise(rb_eRuntimeError, "digest string too long");
    }

    str = rb_str_new(0, (digest_len | 1) * 3 + 2);
    p = RSTRING_PTR(str);

    i = j = 0;
    p[j++] = 'x';

    for (;;) {
        unsigned char byte1, byte2;

        if (i >= digest_len) {
            p[j++] = vowels[seed % 6];
            p[j++] = consonants[16];
            p[j++] = vowels[seed / 6];
            break;
        }

        byte1 = digest[i++];
        p[j++] = vowels[(((byte1 >> 6) & 3) + seed) % 6];
        p[j++] = consonants[(byte1 >> 2) & 15];
        p[j++] = vowels[((byte1 & 3) + (seed / 6)) % 6];

        if (i >= digest_len) {
            break;
        }

        byte2 = digest[i++];
        p[j++] = consonants[(byte2 >> 4) & 15];
        p[j++] = '-';
        p[j++] = consonants[byte2 & 15];

        seed = (seed * 5 + byte1 * 7 + byte2) % 36;
    }

    p[j] = 'x';

    return str;
}

/* Document-method: Digest::bubblebabble
 *
 * call-seq:
 *     Digest.bubblebabble(string) -> bubblebabble_string
 *
 * Returns a BubbleBabble encoded version of a given _string_.
 */
static VALUE
rb_digest_s_bubblebabble(Env *env, VALUE klass, Args args, Block *)
{
    args.ensure_argc_is(env, 1);
    auto str = args.at(0);
    return bubblebabble_str_new(env, str);
}

/* Document-method: Digest::Class::bubblebabble
 *
 * call-seq:
 *     Digest::Class.bubblebabble(string, ...) -> hash_string
 *
 * Returns the BubbleBabble encoded hash value of a given _string_.
 */
static VALUE
rb_digest_class_s_bubblebabble(Env *env, VALUE klass, Args args, Block *)
{
    return bubblebabble_str_new(env, rb_funcallv(klass, "digest"_s, args.size(), args.data()));
}

/* Document-method: Digest::Instance#bubblebabble
 *
 * call-seq:
 *     digest_obj.bubblebabble -> hash_string
 *
 * Returns the resulting hash value in a Bubblebabble encoded form.
 */
static VALUE
rb_digest_instance_bubblebabble(Env *env, VALUE self, Args args, Block *)
{
    return bubblebabble_str_new(env, self->send(env, "digest"_s));
}

/*
 * This module adds some methods to Digest classes to perform
 * BubbleBabble encoding.
 */
Value init_bubblebabble(Env *env, Value self) {
#undef rb_intern
    VALUE rb_mDigest, rb_mDigest_Instance, rb_cDigest_Class;

    rb_require("digest");

#if 0
    rb_mDigest = rb_define_module("Digest");
    rb_mDigest_Instance = rb_define_module_under(rb_mDigest, "Instance");
    rb_cDigest_Class = rb_define_class_under(rb_mDigest, "Class", rb_cObject);
#endif
    rb_mDigest = GlobalEnv::the()->Object()->const_get("Digest"_s);
    if (!rb_mDigest) {
        rb_mDigest = new ModuleObject { "Digest" };
        GlobalEnv::the()->Object()->const_set("Digest"_s, rb_mDigest);
    }
    rb_mDigest_Instance = rb_mDigest->const_get("Instance"_s);
    if (!rb_mDigest_Instance) {
        rb_mDigest_Instance = new ModuleObject { "Instance" };
        rb_mDigest->const_set("Instance"_s, rb_mDigest_Instance);
    }
    rb_cDigest_Class = rb_mDigest->const_get("Class"_s);
    if (!rb_cDigest_Class) {
        rb_cDigest_Class = GlobalEnv::the()->Object()->subclass(env, "Class");
        rb_mDigest->const_set("Class"_s, rb_cDigest_Class);
    }

    rb_define_module_function(rb_mDigest, "bubblebabble", rb_digest_s_bubblebabble, 1);
    rb_define_singleton_method(rb_cDigest_Class, "bubblebabble", rb_digest_class_s_bubblebabble, -1);
    rb_define_method(rb_mDigest_Instance, "bubblebabble", rb_digest_instance_bubblebabble, 0);

    return NilObject::the();
}
