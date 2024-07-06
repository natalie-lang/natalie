#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"
#include "tm/string.hpp"

extern "C" {
#include "onigmo.h"
}

#define ENUMERATE_REGEX_OPTS(C)         \
    C(IgnoreCase, IGNORECASE, 1)        \
    C(Extended, EXTENDED, 2)            \
    C(MultiLine, MULTILINE, 4)          \
    C(FixedEncoding, FIXEDENCODING, 16) \
    C(NoEncoding, NOENCODING, 32)

namespace Natalie {

using namespace TM;

enum RegexOpts {
#define REGEX_OPTS_ENUM_VALUES(cpp_name, ruby_name, bits) cpp_name = bits,
    ENUMERATE_REGEX_OPTS(REGEX_OPTS_ENUM_VALUES)
#undef REGEX_OPTS_ENUM_VALUES
};

class RegexpObject : public Object {
public:
    RegexpObject()
        : Object { Object::Type::Regexp, GlobalEnv::the()->Regexp() } { }

    RegexpObject(ClassObject *klass)
        : Object { Object::Type::Regexp, klass } { }

    RegexpObject(Env *env, const StringObject *pattern, int options = 0)
        : Object { Object::Type::Regexp, GlobalEnv::the()->Regexp() } {
        initialize_internal(env, pattern, options);
    }

    RegexpObject(Env *env, const String pattern, int options = 0)
        : Object { Object::Type::Regexp, GlobalEnv::the()->Regexp() } {
        initialize_internal(env, new StringObject(pattern), options);
    }

    virtual ~RegexpObject() {
        onig_free(m_regex);
    }

    static Value compile(Env *env, Value pattern, Value flags = nullptr, ClassObject *klass = nullptr) {
        if (!klass)
            klass = GlobalEnv::the()->Regexp();
        RegexpObject *regexp = new RegexpObject { klass };
        regexp->send(env, "initialize"_s, { pattern, flags });
        return regexp;
    }

    static EncodingObject *onig_encoding_to_ruby_encoding(OnigEncoding encoding);
    static OnigEncoding ruby_encoding_to_onig_encoding(NonNullPtr<const EncodingObject> encoding);
    static Value last_match(Env *, Value);
    static Value quote(Env *, Value);
    static Value try_convert(Env *, Value);
    static Value regexp_union(Env *, Args);

    static Value literal(Env *env, const char *pattern, int options = 0) {
        auto regex = new RegexpObject(env, pattern, options);
        regex->freeze();
        return regex;
    }

    Value hash(Env *env) {
        assert_initialized(env);
        auto hash = (m_options & ~RegexOpts::NoEncoding & ~RegexOpts::FixedEncoding) + m_pattern->string().djb2_hash();
        return Value::integer(hash);
    }

    void initialize_internal(Env *, const StringObject *, int = 0);

    bool is_initialized() const { return m_options > -1; }

    void assert_initialized(Env *env) const {
        if (!is_initialized())
            env->raise("TypeError", "uninitialized Regexp");
    }

    const StringObject *pattern() const { return m_pattern; }

    EncodingObject *encoding() const {
        if (!is_initialized())
            return EncodingObject::get(Encoding::ASCII_8BIT);

        return onig_encoding_to_ruby_encoding(onig_get_encoding(m_regex));
    }

    int options() const { return m_options; }

    int options(Env *env) const {
        assert_initialized(env);
        return m_options;
    }

    void set_options(String &options) {
        parse_options(options, &m_options);
    }

    void set_options(int options) {
        m_options = options;
    }

    static void parse_options(String &options_string, int *options) {
        for (char *c = const_cast<char *>(options_string.c_str()); *c != '\0'; ++c) {
            switch (*c) {
            case 'i':
                *options |= RegexOpts::IgnoreCase;
                break;
            case 'x':
                *options |= RegexOpts::Extended;
                break;
            case 'm':
                *options |= RegexOpts::MultiLine;
                break;
            default:
                break;
            }
        }
    }

    bool operator==(const RegexpObject &other) const;

    bool casefold(Env *env) const {
        assert_initialized(env);
        return m_options & RegexOpts::IgnoreCase;
    }

    long search(Env *env, const StringObject *string_obj, long start, OnigRegion *region, OnigOptionType options, bool reverse = false);

    bool eq(Env *env, Value other) const {
        assert_initialized(env);
        if (!other->is_regexp()) return false;
        RegexpObject *other_regexp = other->as_regexp();
        other_regexp->assert_initialized(env);
        return *this == *other_regexp;
    }

    bool eqeqeq(Env *env, Value other) {
        assert_initialized(env);
        if (!other->is_string() && !other->is_symbol()) {
            if (!other->respond_to(env, "to_str"_s))
                return false;
            other = other->to_str(env);
        }
        return match(env, other)->is_truthy();
    }

    Value tilde(Env *env) {
        return this->send(env, "=~"_s, { env->global_get("$_"_s) });
    }

    bool is_fixed_encoding() const {
        return m_options & RegexOpts::FixedEncoding;
    }

    bool has_match(Env *env, Value, Value);
    Value initialize(Env *, Value, Value);
    Value inspect(Env *env);
    Value eqtilde(Env *env, Value);
    Value match(Env *env, Value, Value = nullptr, Block * = nullptr);
    Value match_at_byte_offset(Env *env, StringObject *, size_t);
    Value named_captures(Env *) const;
    Value names() const;
    Value source(Env *env) const;
    Value to_s(Env *env) const;

    virtual String dbg_inspect() const override;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RegexpObject %p>", this);
    }

    void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        if (m_pattern)
            visitor.visit(m_pattern);
    }

private:
    friend MatchDataObject;

    regex_t *m_regex { nullptr };
    int m_options { -1 };
    const StringObject *m_pattern { nullptr };
};

}
