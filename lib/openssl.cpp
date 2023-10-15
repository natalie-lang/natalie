#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "natalie.hpp"

using namespace Natalie;

static void OpenSSL_MD_CTX_cleanup(VoidPObject *self) {
    auto mdctx = static_cast<EVP_MD_CTX *>(self->void_ptr());
    EVP_MD_CTX_free(mdctx);
}

static void OpenSSL_raise_error(Env *env, const char *func, ClassObject *klass = nullptr) {
    static auto OpenSSLError = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("OpenSSLError"_s)->as_class();
    if (!klass) klass = OpenSSLError;
    env->raise(klass, "{}: {}", func, ERR_reason_error_string(ERR_get_error()));
}

Value OpenSSL_Digest_update(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());

    Value data = args[0];
    data->assert_type(env, Object::Type::String, "String");

    if (!EVP_DigestUpdate(mdctx, reinterpret_cast<const unsigned char *>(data->as_string()->c_str()), data->as_string()->string().size()))
        OpenSSL_raise_error(env, "EVP_DigestUpdate");

    return self;
}

Value OpenSSL_Digest_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto name = args.at(0);
    auto digest_klass = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("Digest"_s);
    if (name->is_a(env, digest_klass))
        name = name->send(env, "name"_s);
    if (!name->is_string())
        env->raise("TypeError", "wrong argument type {} (expected OpenSSL/Digest)", name->klass()->inspect_str());

    const EVP_MD *md = EVP_get_digestbyname(name->as_string()->c_str());
    if (!md)
        env->raise("RuntimeError", "Unsupported digest algorithm ({}).: unknown object name", name->as_string()->string());

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!EVP_DigestInit_ex(mdctx, md, nullptr))
        OpenSSL_raise_error(env, "EVP_DigestInit_ex");

    self->ivar_set(env, "@name"_s, name->as_string()->upcase(env, nullptr, nullptr));
    self->ivar_set(env, "@mdctx"_s, new VoidPObject { mdctx, OpenSSL_MD_CTX_cleanup });

    if (args.size() == 2)
        OpenSSL_Digest_update(env, self, { args[1] }, nullptr);

    return self;
}

Value OpenSSL_Digest_block_length(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());
    const int block_size = EVP_MD_CTX_block_size(mdctx);
    return IntegerObject::create(block_size);
}

Value OpenSSL_Digest_reset(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    const EVP_MD *md = EVP_get_digestbyname(self->send(env, "name"_s)->as_string()->c_str());
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());

    if (!EVP_MD_CTX_reset(mdctx))
        OpenSSL_raise_error(env, "EVP_MD_CTX_reset");
    if (!EVP_DigestInit_ex(mdctx, md, nullptr))
        OpenSSL_raise_error(env, "EVP_DigestInit_ex");

    return self;
}

Value OpenSSL_Digest_digest(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());

    if (args.size() == 1)
        OpenSSL_Digest_update(env, self, { args[0] }, nullptr);

    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    if (!EVP_DigestFinal_ex(mdctx, buf, &md_len))
        OpenSSL_raise_error(env, "EVP_DigestFinal_ex");
    OpenSSL_Digest_reset(env, self, {}, nullptr);

    return new StringObject { reinterpret_cast<const char *>(buf), md_len };
}

Value OpenSSL_Digest_digest_length(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());
    const int digest_length = EVP_MD_CTX_size(mdctx);
    return IntegerObject::create(digest_length);
}

Value OpenSSL_KDF_pbkdf2_hmac(Env *env, Value self, Args args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_is(env, 1);
    auto pass = args.at(0)->to_str(env);
    if (!kwargs) kwargs = new HashObject {};
    env->ensure_no_missing_keywords(kwargs, { "salt", "iterations", "length", "hash" });
    auto salt = kwargs->remove(env, "salt"_s)->to_str(env);
    auto iterations = kwargs->remove(env, "iterations"_s)->to_int(env);
    auto length = kwargs->remove(env, "length"_s)->to_int(env);
    auto hash = kwargs->remove(env, "hash"_s);
    auto digest_klass = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("Digest"_s);
    if (!hash->is_a(env, digest_klass))
        hash = Object::_new(env, digest_klass, { hash }, nullptr);
    hash = hash->send(env, "name"_s);
    env->ensure_no_extra_keywords(kwargs);

    const EVP_MD *md = EVP_get_digestbyname(hash->as_string()->c_str());
    if (!md)
        env->raise("RuntimeError", "Unsupported digest algorithm ({}).: unknown object name", hash->as_string()->string());
    const size_t out_size = length->as_integer()->to_nat_int_t();
    unsigned char out[out_size];
    int result = PKCS5_PBKDF2_HMAC(pass->as_string()->c_str(), pass->as_string()->bytesize(),
        reinterpret_cast<const unsigned char *>(salt->as_string()->c_str()), salt->as_string()->bytesize(),
        iterations->as_integer()->to_nat_int_t(),
        md,
        out_size, out);
    if (!result) {
        auto OpenSSL = GlobalEnv::the()->Object()->const_get("OpenSSL"_s);
        auto KDF = OpenSSL->const_get("KDF"_s);
        auto KDFError = KDF->const_get("KDFError"_s);
        OpenSSL_raise_error(env, "PKCS5_PBKDF2_HMAC", KDFError->as_class());
    }

    return new StringObject { reinterpret_cast<char *>(out), out_size, EncodingObject::get(Encoding::ASCII_8BIT) };
}

Value init(Env *env, Value self) {
    auto OpenSSL = GlobalEnv::the()->Object()->const_get("OpenSSL"_s);
    if (!OpenSSL) {
        OpenSSL = new ModuleObject { "OpenSSL" };
        GlobalEnv::the()->Object()->const_set("OpenSSL"_s, OpenSSL);
    }

    // OpenSSL < 3.0 does not have a OPENSSL_VERSION_STR
    const auto openssl_version_major = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 28) & 0xFF);
    const auto openssl_version_minor = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 20) & 0xFF);
    const auto openssl_version_patchlevel = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 12) & 0xFF);
    StringObject *VERSION = new StringObject {
        TM::String::format("{}.{}.{}", openssl_version_major, openssl_version_minor, openssl_version_patchlevel)
    };
    OpenSSL->const_set("VERSION"_s, VERSION);

    auto Digest = OpenSSL->const_get("Digest"_s);
    if (!Digest) {
        Digest = GlobalEnv::the()->Object()->subclass(env, "Digest");
        OpenSSL->const_set("Digest"_s, Digest);
    }
    Digest->define_method(env, "initialize"_s, OpenSSL_Digest_initialize, -1);
    Digest->define_method(env, "block_length"_s, OpenSSL_Digest_block_length, 0);
    Digest->define_method(env, "digest"_s, OpenSSL_Digest_digest, -1);
    Digest->define_method(env, "digest_length"_s, OpenSSL_Digest_digest_length, 0);
    Digest->define_method(env, "reset"_s, OpenSSL_Digest_reset, 0);
    Digest->define_method(env, "update"_s, OpenSSL_Digest_update, 1);
    Digest->define_method(env, "<<"_s, OpenSSL_Digest_update, 1);

    auto KDF = OpenSSL->const_get("KDF"_s);
    if (!KDF) {
        KDF = new ModuleObject { "KDF" };
        OpenSSL->const_set("KDF"_s, KDF);
    }
    KDF->define_singleton_method(env, "pbkdf2_hmac"_s, OpenSSL_KDF_pbkdf2_hmac, -1);

    return NilObject::the();
}

Value OpenSSL_Random_random_bytes(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    Value length = args[0];
    const auto to_int = "to_int"_s;
    if (!length->is_integer() && length->respond_to(env, to_int))
        length = length->send(env, to_int);
    length->assert_type(env, ObjectType::Integer, "Integer");
    const auto num = static_cast<int>(length->as_integer()->to_nat_int_t());
    if (num < 0)
        env->raise("ArgumentError", "negative string size (or size too big)");

    unsigned char buf[num];
    if (RAND_bytes(buf, num) != 1)
        OpenSSL_raise_error(env, "RAND_bytes");

    return new StringObject { reinterpret_cast<char *>(buf), static_cast<size_t>(num), EncodingObject::get(Encoding::ASCII_8BIT) };
}
