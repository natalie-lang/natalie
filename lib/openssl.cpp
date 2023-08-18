#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "tm/defer.hpp"

#include "natalie.hpp"

using namespace Natalie;

Value init(Env *env, Value self) {
    ModuleObject *OpenSSL = new ModuleObject { "OpenSSL" };
    GlobalEnv::the()->Object()->const_set("OpenSSL"_s, OpenSSL);

    // OpenSSL < 3.0 does not have a OPENSSL_VERSION_STR
    const auto openssl_version_major = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 28) & 0xFF);
    const auto openssl_version_minor = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 20) & 0xFF);
    const auto openssl_version_patchlevel = static_cast<nat_int_t>((OPENSSL_VERSION_NUMBER >> 12) & 0xFF);
    StringObject *VERSION = new StringObject {
        TM::String::format("{}.{}.{}", openssl_version_major, openssl_version_minor, openssl_version_patchlevel)
    };
    OpenSSL->const_set("VERSION"_s, VERSION);

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
    if (RAND_bytes(buf, num) != 1) {
        const auto err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, 256);
        env->raise("RuntimeError", err_buf);
    }

    return new StringObject { reinterpret_cast<char *>(buf), static_cast<size_t>(num), EncodingObject::get(Encoding::ASCII_8BIT) };
}

static inline Value digest_wrapper(Env *env, Args args, const char *name) {
    args.ensure_argc_is(env, 1);
    Value data = args[0];
    data->assert_type(env, Object::Type::String, "String");

    const EVP_MD *md = EVP_get_digestbyname(name);
    if (!md)
        env->raise("RuntimeError", "Unsupported digest algorithm ({}).: unknown object name", name);

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    auto mdctx_destructor = TM::Defer([&]() { EVP_MD_CTX_free(mdctx); });
    if (!EVP_DigestInit_ex(mdctx, md, nullptr))
        env->raise("RuntimeError", "Internal OpenSSL error");
    if (!EVP_DigestUpdate(mdctx, reinterpret_cast<const unsigned char *>(data->as_string()->c_str()), data->as_string()->string().size()))
        env->raise("RuntimeError", "Internal OpenSSL error");
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    if (!EVP_DigestFinal_ex(mdctx, buf, &md_len))
        env->raise("RuntimeError", "Internal OpenSSL error");

    return new StringObject { reinterpret_cast<const char *>(buf), md_len };
}

Value OpenSSL_Digest_SHA1_digest(Env *env, Value self, Args args, Block *) {
    return digest_wrapper(env, args, "SHA1");
}

Value OpenSSL_Digest_SHA256_digest(Env *env, Value self, Args args, Block *) {
    return digest_wrapper(env, args, "SHA256");
}

Value OpenSSL_Digest_SHA384_digest(Env *env, Value self, Args args, Block *) {
    return digest_wrapper(env, args, "SHA384");
}

Value OpenSSL_Digest_SHA512_digest(Env *env, Value self, Args args, Block *) {
    return digest_wrapper(env, args, "SHA512");
}
