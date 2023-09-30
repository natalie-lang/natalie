#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "tm/defer.hpp"

#include "natalie.hpp"

using namespace Natalie;

Value OpenSSL_Digest_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto name = args.at(0);
    auto digest_klass = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("Digest"_s);
    if (name->is_a(env, digest_klass))
        name = name->send(env, "name"_s);
    if (!name->is_string())
        env->raise("TypeError", "wrong argument type {} (expected OpenSSL/Digest)", name->klass()->inspect_str());

    const EVP_MD *md = EVP_get_digestbyname(name->as_string()->c_str());
    if (!md)
        env->raise("RuntimeError", "Unsupported digest algorithm ({}).: unknown object name", name->as_string()->string());

    self->ivar_set(env, "@name"_s, name->as_string()->upcase(env, nullptr, nullptr));

    return self;
}

Value OpenSSL_Digest_block_length(Env *env, Value self, Args args, Block *) {
    auto name = self->send(env, "name"_s);
    name->assert_type(env, Object::Type::String, "String");

    args.ensure_argc_is(env, 0);
    const EVP_MD *md = EVP_get_digestbyname(name->as_string()->c_str());
    if (!md)
        env->raise("RuntimeError", "Unsupported digest algorithm ({}).: unknown object name", name);

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    auto mdctx_destructor = TM::Defer([&]() { EVP_MD_CTX_free(mdctx); });
    if (!EVP_DigestInit_ex(mdctx, md, nullptr))
        env->raise("RuntimeError", "Internal OpenSSL error");

    const int block_size = EVP_MD_CTX_block_size(mdctx);
    return IntegerObject::create(block_size);
}

Value OpenSSL_Digest_digest(Env *env, Value self, Args args, Block *) {
    auto name = self->send(env, "name"_s);
    name->assert_type(env, Object::Type::String, "String");

    args.ensure_argc_is(env, 1);
    Value data = args[0];
    data->assert_type(env, Object::Type::String, "String");

    const EVP_MD *md = EVP_get_digestbyname(name->as_string()->c_str());
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
    Digest->define_method(env, "initialize"_s, OpenSSL_Digest_initialize, 1);
    Digest->define_method(env, "block_length"_s, OpenSSL_Digest_block_length, 0);
    Digest->define_method(env, "digest"_s, OpenSSL_Digest_digest, 1);

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
