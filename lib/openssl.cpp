#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "natalie.hpp"

using namespace Natalie;

static void OpenSSL_CIPHER_CTX_cleanup(VoidPObject *self) {
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->void_ptr());
    EVP_CIPHER_CTX_free(ctx);
}

static void OpenSSL_MD_CTX_cleanup(VoidPObject *self) {
    auto mdctx = static_cast<EVP_MD_CTX *>(self->void_ptr());
    EVP_MD_CTX_free(mdctx);
}

static void OpenSSL_SSL_CTX_cleanup(VoidPObject *self) {
    auto ctx = static_cast<SSL_CTX *>(self->void_ptr());
    SSL_CTX_free(ctx);
}

static void OpenSSL_SSL_cleanup(VoidPObject *self) {
    auto ssl = static_cast<SSL *>(self->void_ptr());
    SSL_free(ssl);
}

static void OpenSSL_X509_NAME_cleanup(VoidPObject *self) {
    auto name = static_cast<X509_NAME *>(self->void_ptr());
    X509_NAME_free(name);
}

static void OpenSSL_raise_error(Env *env, const char *func, ClassObject *klass = nullptr) {
    static auto OpenSSLError = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("OpenSSLError"_s)->as_class();
    if (!klass) klass = OpenSSLError;
    env->raise(klass, "{}: {}", func, ERR_reason_error_string(ERR_get_error()));
}

static void OpenSSL_Cipher_raise_error(Env *env, const char *func) {
    static auto OpenSSL = GlobalEnv::the()->Object()->const_get("OpenSSL"_s);
    static auto Cipher = OpenSSL->const_get("Cipher"_s);
    static auto CipherError = Cipher->const_get("CipherError"_s);
    OpenSSL_raise_error(env, func, CipherError->as_class());
}

static void OpenSSL_X509_Name_raise_error(Env *env, const char *func) {
    static auto OpenSSL = GlobalEnv::the()->Object()->const_get("OpenSSL"_s);
    static auto X509 = OpenSSL->const_get("X509"_s);
    static auto NameError = X509->const_get("NameError"_s);
    OpenSSL_raise_error(env, func, NameError->as_class());
}

Value OpenSSL_fixed_length_secure_compare(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 2);
    auto a = args[0]->to_str(env);
    auto b = args[1]->to_str(env);
    const auto len = a->bytesize();
    if (b->bytesize() != len)
        env->raise("ArgumentError", "inputs must be of equal length");
    if (CRYPTO_memcmp(a->c_str(), b->c_str(), len) == 0)
        return TrueObject::the();
    return FalseObject::the();
}

static void OpenSSL_Cipher_ciphers_add_cipher(const OBJ_NAME *cipher_meth, void *arg) {
    auto result = static_cast<ArrayObject *>(arg);
    result->push(new StringObject { cipher_meth->name });
}

Value OpenSSL_Cipher_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto name = args[0]->to_str(env);
    const EVP_CIPHER *cipher = EVP_get_cipherbyname(name->c_str());
    if (!cipher)
        env->raise("RuntimeError", "unsupported cipher algorithm ({})", name->string());
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        OpenSSL_Cipher_raise_error(env, "EVP_CIPHER_CTX");
    if (!EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        OpenSSL_Cipher_raise_error(env, "EVP_EncryptInit_ex");
    }
    self->ivar_set(env, "@ctx"_s, new VoidPObject { ctx, OpenSSL_CIPHER_CTX_cleanup });

    return self;
}

Value OpenSSL_Cipher_block_size(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const nat_int_t block_size = EVP_CIPHER_CTX_block_size(ctx);
    return Value::integer(block_size);
}

Value OpenSSL_Cipher_decrypt(Env *env, Value self, Args args, Block *) {
    // NATFIXME: There is deprecated behaviour when calling with arguments. Let's not try to reproduce that for now
    // warning: arguments for OpenSSL::Cipher#encrypt and OpenSSL::Cipher#decrypt were deprecated; use OpenSSL::Cipher#pkcs5_keyivgen to derive key and IV
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    EVP_CipherInit_ex(ctx, nullptr, nullptr, nullptr, nullptr, 0);
    return self;
}

Value OpenSSL_Cipher_encrypt(Env *env, Value self, Args args, Block *) {
    // NATFIXME: There is deprecated behaviour when calling with arguments. Let's not try to reproduce that for now
    // warning: arguments for OpenSSL::Cipher#encrypt and OpenSSL::Cipher#decrypt were deprecated; use OpenSSL::Cipher#pkcs5_keyivgen to derive key and IV
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    EVP_CipherInit_ex(ctx, nullptr, nullptr, nullptr, nullptr, 1);
    return self;
}

Value OpenSSL_Cipher_final(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    int size = EVP_CIPHER_CTX_block_size(ctx);
    TM::String buf(size, '\0');
    if (!EVP_CipherFinal_ex(ctx, reinterpret_cast<unsigned char *>(&buf[0]), &size))
        OpenSSL_Cipher_raise_error(env, "EVP_CipherFinal_ex");
    buf.truncate(size);
    return new StringObject { std::move(buf), EncodingObject::get(Encoding::ASCII_8BIT) };
}

Value OpenSSL_Cipher_iv_set(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto iv = args[0]->to_str(env);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const EVP_CIPHER *e = EVP_CIPHER_CTX_cipher(ctx);
    const size_t iv_len = EVP_CIPHER_iv_length(e);
    if (iv->bytesize() != iv_len)
        env->raise("ArgumentError", "iv must be {} bytes", iv_len);
    if (!EVP_CipherInit_ex(ctx, nullptr, nullptr, nullptr, reinterpret_cast<const unsigned char *>(iv->c_str()), -1))
        OpenSSL_Cipher_raise_error(env, "EVP_CipherInit_ex");
    return iv;
}

Value OpenSSL_Cipher_iv_len(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const EVP_CIPHER *e = EVP_CIPHER_CTX_cipher(ctx);
    const nat_int_t iv_len = EVP_CIPHER_iv_length(e);
    return Value::integer(iv_len);
}

Value OpenSSL_Cipher_key_set(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto key = args[0]->to_str(env);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const EVP_CIPHER *e = EVP_CIPHER_CTX_cipher(ctx);
    const size_t key_len = EVP_CIPHER_key_length(e);
    if (key->bytesize() != key_len)
        env->raise("ArgumentError", "key must be {} bytes", key_len);
    if (!EVP_CipherInit_ex(ctx, nullptr, nullptr, reinterpret_cast<const unsigned char *>(key->c_str()), nullptr, -1))
        OpenSSL_Cipher_raise_error(env, "EVP_CipherInit_ex");
    return key;
}

Value OpenSSL_Cipher_key_len(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const EVP_CIPHER *e = EVP_CIPHER_CTX_cipher(ctx);
    const nat_int_t key_len = EVP_CIPHER_key_length(e);
    return Value::integer(key_len);
}

Value OpenSSL_Cipher_update(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1); // NATFXIME: Support buffer argument
    auto data = args[0]->to_str(env);
    auto ctx = static_cast<EVP_CIPHER_CTX *>(self->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    const auto block_size = EVP_CIPHER_CTX_block_size(ctx);
    TM::String buf(data->bytesize() + block_size, '\0'); // Overallocation, so it should always fit
    int size = buf.size();
    if (!EVP_CipherUpdate(ctx, reinterpret_cast<unsigned char *>(&buf[0]), &size, reinterpret_cast<const unsigned char *>(data->c_str()), data->bytesize()))
        OpenSSL_Cipher_raise_error(env, "EVP_CipherUpdate");
    buf.truncate(size);
    return new StringObject { std::move(buf), EncodingObject::get(Encoding::ASCII_8BIT) };
}

Value OpenSSL_Cipher_ciphers(Env *env, Value self, Args args, Block *) {
    auto result = new ArrayObject {};
    OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, OpenSSL_Cipher_ciphers_add_cipher, result);
    return result;
}

Value OpenSSL_Digest_update(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto mdctx = static_cast<EVP_MD_CTX *>(self->ivar_get(env, "@mdctx"_s)->as_void_p()->void_ptr());

    auto data = args[0]->to_str(env);

    if (!EVP_DigestUpdate(mdctx, reinterpret_cast<const unsigned char *>(data->c_str()), data->string().size()))
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

Value OpenSSL_HMAC_digest(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 3);
    auto digest_klass = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("Digest"_s);
    auto digest = Object::_new(env, digest_klass, { args[0] }, nullptr);
    auto key = args[1]->to_str(env);
    auto data = args[2]->to_str(env);
    const EVP_MD *evp_md = EVP_get_digestbyname(digest->send(env, "name"_s)->as_string()->c_str());
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    auto res = HMAC(evp_md, key->c_str(), key->bytesize(), reinterpret_cast<const unsigned char *>(data->c_str()), data->bytesize(), md, &md_len);
    if (!res)
        OpenSSL_raise_error(env, "HMAC");
    return new StringObject { reinterpret_cast<const char *>(md), md_len };
}

Value OpenSSL_SSL_SSLContext_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0); // NATFIXME: Add deprecated version argument
    SSL_CTX *ctx = SSL_CTX_new(TLS_method());
    if (!ctx)
        OpenSSL_raise_error(env, "SSL_CTX_new");
    self->ivar_set(env, "@ctx"_s, new VoidPObject { ctx, OpenSSL_SSL_CTX_cleanup });
    return self;
}

Value OpenSSL_SSL_SSLSocket_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto io = args.at(0);
    if (!io->is_io())
        env->raise("TypeError", "wrong argument type {} (expected File)", io->klass()->inspect_str());
    auto context = args.at(1, nullptr);
    auto SSLContext = GlobalEnv::the()->Object()->const_get("OpenSSL"_s)->const_get("SSL"_s)->const_get("SSLContext"_s);
    if (!context || context->is_nil()) {
        context = Object::_new(env, SSLContext, {}, nullptr);
    } else {
        if (!context->is_a(env, SSLContext->as_class()))
            env->raise("TypeError", "wrong argument type {} (expected OpenSSL/SSL/CTX)", context->klass()->inspect_str());
    }
    auto *ctx = static_cast<SSL_CTX *>(context->ivar_get(env, "@ctx"_s)->as_void_p()->void_ptr());
    SSL *ssl = SSL_new(ctx);
    if (!ssl)
        OpenSSL_raise_error(env, "SSL_new");
    self->ivar_set(env, "@context"_s, context);
    self->ivar_set(env, "@io"_s, io);
    self->ivar_set(env, "@ssl"_s, new VoidPObject { ssl, OpenSSL_SSL_cleanup });
    return self;
}

Value OpenSSL_SSL_SSLSocket_close(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ssl = static_cast<SSL *>(self->ivar_get(env, "@ssl"_s)->as_void_p()->void_ptr());
    SSL_shutdown(ssl);
    return self;
}

Value OpenSSL_SSL_SSLSocket_connect(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto ssl = static_cast<SSL *>(self->ivar_get(env, "@ssl"_s)->as_void_p()->void_ptr());
    auto fd = self->ivar_get(env, "@io"_s)->as_io()->fileno();
    const auto flags = fcntl(fd, F_GETFL);
    if (!SSL_set_fd(ssl, fd))
        OpenSSL_raise_error(env, "SSL_set_fd");
    if (!SSL_connect(ssl))
        OpenSSL_raise_error(env, "SSL_connect");
    return self;
}

Value OpenSSL_SSL_SSLSocket_read(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0); // NATFIXME: This probably supports a buffer
    auto ssl = static_cast<SSL *>(self->ivar_get(env, "@ssl"_s)->as_void_p()->void_ptr());
    constexpr size_t buf_size = 1024;
    TM::String buf(buf_size, '\0');
    auto result = new StringObject {};
    int bytes_read;
    while ((bytes_read = SSL_read(ssl, &buf[0], buf_size)) > 0) {
        result->append(buf.c_str(), bytes_read);
    }
    return result;
}

Value OpenSSL_SSL_SSLSocket_write(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto str = args.at(0)->to_s(env);
    auto ssl = static_cast<SSL *>(self->ivar_get(env, "@ssl"_s)->as_void_p()->void_ptr());
    const auto size = SSL_write(ssl, str->c_str(), str->bytesize());
    if (size < 0)
        OpenSSL_raise_error(env, "SSL_write");
    return Value::integer(size);
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

Value OpenSSL_KDF_scrypt(Env *env, Value self, Args args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_is(env, 1);
    auto pass = args.at(0)->to_str(env);
    env->ensure_no_missing_keywords(kwargs, { "salt", "N", "r", "p", "length" });
    auto salt = kwargs->remove(env, "salt"_s)->to_str(env);
    auto N = kwargs->remove(env, "N"_s)->to_int(env);
    auto r = kwargs->remove(env, "r"_s)->to_int(env);
    auto p = kwargs->remove(env, "p"_s)->to_int(env);
    auto length = kwargs->remove(env, "length"_s)->to_int(env);
    if (length->is_negative() || length->is_bignum())
        env->raise("ArgumentError", "negative string size (or size too big)");
    env->ensure_no_extra_keywords(kwargs);

    auto OpenSSL = GlobalEnv::the()->Object()->const_get("OpenSSL"_s);
    auto KDF = OpenSSL->const_get("KDF"_s);
    auto KDFError = KDF->const_get("KDFError"_s);
    auto pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, nullptr);
    Defer pctx_free { [&pctx]() { EVP_PKEY_CTX_free(pctx); } };
    unsigned char out[length->to_nat_int_t()];
    size_t outlen = sizeof(out);
    if (EVP_PKEY_derive_init(pctx) <= 0) {
        OpenSSL_raise_error(env, "EVP_PKEY_derive_init", KDFError->as_class());
    }
    if (EVP_PKEY_CTX_set1_pbe_pass(pctx, pass->c_str(), pass->bytesize()) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }
    if (EVP_PKEY_CTX_set1_scrypt_salt(pctx, reinterpret_cast<const unsigned char *>(salt->c_str()), salt->bytesize()) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }
    if (EVP_PKEY_CTX_set_scrypt_N(pctx, N->to_nat_int_t()) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }
    if (EVP_PKEY_CTX_set_scrypt_r(pctx, r->to_nat_int_t()) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }
    if (EVP_PKEY_CTX_set_scrypt_p(pctx, p->to_nat_int_t()) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }
    if (EVP_PKEY_derive(pctx, out, &outlen) <= 0) {
        OpenSSL_raise_error(env, "EVP_PBE_scrypt", KDFError->as_class());
    }

    return new StringObject { reinterpret_cast<const char *>(out), outlen };
}

Value init(Env *env, Value self) {
    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS, nullptr);

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

    OpenSSL->const_set("OPENSSL_VERSION"_s, new StringObject { OPENSSL_VERSION_TEXT });
    OpenSSL->const_set("OPENSSL_VERSION_NUMBER"_s, new IntegerObject { OPENSSL_VERSION_NUMBER });
    OpenSSL->define_singleton_method(env, "fixed_length_secure_compare"_s, OpenSSL_fixed_length_secure_compare, 2);

    auto Cipher = OpenSSL->const_get("Cipher"_s);
    if (!Cipher) {
        Cipher = GlobalEnv::the()->Object()->subclass(env, "Cipher");
        OpenSSL->const_set("Cipher"_s, Cipher);
    }
    Cipher->define_method(env, "initialize"_s, OpenSSL_Cipher_initialize, 1);
    Cipher->define_method(env, "block_size"_s, OpenSSL_Cipher_block_size, 0);
    Cipher->define_method(env, "decrypt"_s, OpenSSL_Cipher_decrypt, 0);
    Cipher->define_method(env, "encrypt"_s, OpenSSL_Cipher_encrypt, 0);
    Cipher->define_method(env, "final"_s, OpenSSL_Cipher_final, 0);
    Cipher->define_method(env, "iv="_s, OpenSSL_Cipher_iv_set, 1);
    Cipher->define_method(env, "iv_len"_s, OpenSSL_Cipher_iv_len, 0);
    Cipher->define_method(env, "key="_s, OpenSSL_Cipher_key_set, 1);
    Cipher->define_method(env, "key_len"_s, OpenSSL_Cipher_key_len, 0);
    Cipher->define_method(env, "update"_s, OpenSSL_Cipher_update, 1);
    Cipher->define_singleton_method(env, "ciphers"_s, OpenSSL_Cipher_ciphers, 0);

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

    auto HMAC = OpenSSL->const_get("HMAC"_s);
    if (!HMAC) {
        HMAC = GlobalEnv::the()->Object()->subclass(env, "HMAC");
        OpenSSL->const_set("HMAC"_s, HMAC);
    }
    HMAC->define_singleton_method(env, "digest"_s, OpenSSL_HMAC_digest, 3);

    auto KDF = OpenSSL->const_get("KDF"_s);
    if (!KDF) {
        KDF = new ModuleObject { "KDF" };
        OpenSSL->const_set("KDF"_s, KDF);
    }
    KDF->define_singleton_method(env, "pbkdf2_hmac"_s, OpenSSL_KDF_pbkdf2_hmac, -1);
    KDF->define_singleton_method(env, "scrypt"_s, OpenSSL_KDF_scrypt, -1);

    return NilObject::the();
}

Value OpenSSL_Random_random_bytes(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    Value length = args[0]->to_int(env);
    const auto num = static_cast<int>(length->as_integer()->to_nat_int_t());
    if (num < 0)
        env->raise("ArgumentError", "negative string size (or size too big)");

    unsigned char buf[num];
    if (RAND_bytes(buf, num) != 1)
        OpenSSL_raise_error(env, "RAND_bytes");

    return new StringObject { reinterpret_cast<char *>(buf), static_cast<size_t>(num), EncodingObject::get(Encoding::ASCII_8BIT) };
}

Value OpenSSL_X509_Name_add_entry(Env *env, Value self, Args args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    auto kwarg_loc = kwargs ? kwargs->remove(env, "loc"_s) : nullptr;
    auto kwarg_set = kwargs ? kwargs->remove(env, "set"_s) : nullptr;
    env->ensure_no_extra_keywords(kwargs);
    args.ensure_argc_between(env, 2, 3);
    auto oid = args.at(0)->to_str(env);
    auto value = args.at(1)->to_str(env);
    auto type = args.at(2, nullptr);
    if (type && !type->is_nil()) {
        type = type->to_int(env);
    } else {
        auto OBJECT_TYPE_TEMPLATE = self->klass()->const_get("OBJECT_TYPE_TEMPLATE"_s)->as_hash();
        type = OBJECT_TYPE_TEMPLATE->ref(env, oid);
    }
    auto name = static_cast<X509_NAME *>(self->ivar_get(env, "@name"_s)->as_void_p()->void_ptr());
    int loc = kwarg_loc && !kwarg_loc->is_nil() ? IntegerObject::convert_to_nat_int_t(env, kwarg_loc) : -1;
    int set = kwarg_set && !kwarg_set->is_nil() ? IntegerObject::convert_to_nat_int_t(env, kwarg_set) : 0;
    if (!X509_NAME_add_entry_by_txt(name, oid->c_str(), IntegerObject::convert_to_nat_int_t(env, type), reinterpret_cast<const unsigned char *>(value->c_str()), value->bytesize(), loc, set))
        OpenSSL_X509_Name_raise_error(env, "X509_NAME_add_entry_by_txt");
    return self;
}

Value OpenSSL_X509_Name_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 2);
    X509_NAME *name = X509_NAME_new();
    if (!name)
        OpenSSL_X509_Name_raise_error(env, "X509_NAME_new");
    self->ivar_set(env, "@name"_s, new VoidPObject { name, OpenSSL_X509_NAME_cleanup });
    if (args.size() > 0) {
        HashObject *lookup = self->klass()->const_get("OBJECT_TYPE_TEMPLATE"_s)->as_hash();
        if (args.size() >= 2 && !args.at(1)->is_nil())
            lookup = args.at(1)->to_hash(env);
        for (auto entry : *args.at(0)->to_ary(env)) {
            ArrayObject *add_entry_args = entry->to_ary(env);
            if (args.size() >= 2 && add_entry_args->size() == 2) {
                add_entry_args = add_entry_args->dup(env)->as_array();
                add_entry_args->push(lookup->ref(env, add_entry_args->at(0)));
            }
            OpenSSL_X509_Name_add_entry(env, self, Args(add_entry_args), nullptr);
        }
    }
    return self;
}

Value OpenSSL_X509_Name_to_a(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto name = static_cast<X509_NAME *>(self->ivar_get(env, "@name"_s)->as_void_p()->void_ptr());
    const size_t size = X509_NAME_entry_count(name);
    auto result = new ArrayObject { size };
    for (size_t i = 0; i < size; i++) {
        X509_NAME_ENTRY *name_entry = X509_NAME_get_entry(name, i);
        auto entry_result = new ArrayObject { 3 };

        ASN1_OBJECT *obj = X509_NAME_ENTRY_get_object(name_entry);
        if (!obj)
            OpenSSL_X509_Name_raise_error(env, "X509_NAME_ENTRY_get_object");
        const auto nid = OBJ_obj2nid(obj);
        if (nid == NID_undef)
            OpenSSL_X509_Name_raise_error(env, "OBJ_obj2nid");
        const char *sn = OBJ_nid2sn(nid);
        if (!sn)
            OpenSSL_X509_Name_raise_error(env, "OBJ_nid2sn");
        entry_result->push(new StringObject { sn });

        ASN1_STRING *data = X509_NAME_ENTRY_get_data(name_entry);
        if (!data)
            OpenSSL_X509_Name_raise_error(env, "X509_NAME_ENTRY_get_data");
        entry_result->push(new StringObject { reinterpret_cast<const char *>(ASN1_STRING_get0_data(data)), static_cast<size_t>(ASN1_STRING_length(data)) });

        entry_result->push(IntegerObject::create(ASN1_STRING_type(data)));

        result->push(entry_result);
    }
    return result;
}

Value OpenSSL_X509_Name_to_s(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto format = args.at(0, nullptr);
    auto name = static_cast<X509_NAME *>(self->ivar_get(env, "@name"_s)->as_void_p()->void_ptr());
    if (!format || format->is_nil()) {
        char *str = X509_NAME_oneline(name, nullptr, 0);
        if (!str)
            OpenSSL_X509_Name_raise_error(env, "X509_NAME_oneline");
        auto result = new StringObject { str };
        free(str);
        return result;
    }
    int flags = IntegerObject::convert_to_nat_int_t(env, format);
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
        OpenSSL_raise_error(env, "BIO_new_mem_buf");
    Defer bio_free { [&bio]() { BIO_vfree(bio); } };
    if (!X509_NAME_print_ex(bio, name, 0, flags))
        OpenSSL_X509_Name_raise_error(env, "X509_NAME_print_ex");
    char *mem;
    auto size = BIO_get_mem_data(bio, &mem);
    if (size < 0)
        OpenSSL_X509_Name_raise_error(env, "BIO_get_mem_data");
    return new StringObject { mem, static_cast<size_t>(size) };
}
