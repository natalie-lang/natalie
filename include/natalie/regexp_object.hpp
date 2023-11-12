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

    RegexpObject(Env *env, const String &pattern, int options = 0)
        : Object { Object::Type::Regexp, GlobalEnv::the()->Regexp() } {
        initialize(env, pattern, options);
    }

    virtual ~RegexpObject() {
        onig_free(m_regex);
    }

    static Value compile(Env *env, Value pattern, Value flags = nullptr, ClassObject *klass = nullptr) {
        pattern->assert_type(env, Object::Type::String, "String");
        if (!klass)
            klass = GlobalEnv::the()->Regexp();
        RegexpObject *regexp = new RegexpObject { klass };
        regexp->send(env, "initialize"_s, { pattern, flags });
        return regexp;
    }

    static EncodingObject *onig_encoding_to_ruby_encoding(const OnigEncoding encoding);
    static OnigEncoding ruby_encoding_to_onig_encoding(const Value encoding);
    static Value last_match(Env *, Value);
    static Value quote(Env *, Value);
    static Value try_convert(Env *, Value);

    static Value literal(Env *env, const char *pattern, int options = 0) {
        auto regex = new RegexpObject(env, pattern, options);
        regex->freeze();
        return regex;
    }

    Value hash(Env *env) {
        assert_initialized(env);
        auto hash = (m_options & ~RegexOpts::NoEncoding) + m_pattern.djb2_hash();
        return Value::integer(hash);
    }

    void initialize(Env *env, const String &pattern, int options = 0) {
        regex_t *regex;
        OnigErrorInfo einfo;
        m_pattern = pattern;
        UChar *pat = (UChar *)(m_pattern.c_str());
        // NATFIXME: Fully support character encodings in capture groups
        int result = onig_new(&regex, pat, pat + m_pattern.length(),
            options, ONIG_ENCODING_UTF_8, ONIG_SYNTAX_DEFAULT, &einfo);
        if (result != ONIG_NORMAL) {
            OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
            onig_error_code_to_str(s, result, &einfo);
            env->raise("SyntaxError", (char *)s);
        }
        m_regex = regex;
        m_options = options;
    }

    bool is_initialized() const { return m_options > -1; }

    void assert_initialized(Env *env) const {
        if (!is_initialized())
            env->raise("TypeError", "uninitialized Regexp");
    }

    const String &pattern() const { return m_pattern; }

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

    bool operator==(const RegexpObject &other) const {
        return m_pattern == other.m_pattern && (m_options | RegexOpts::NoEncoding) == (other.m_options | RegexOpts::NoEncoding);
        // /n encoding option is ignored when doing == in ruby MRI
    }

    bool casefold(Env *env) const {
        assert_initialized(env);
        return m_options & RegexOpts::IgnoreCase;
    }

    int search(const TM::String &string, int start, OnigRegion *region, OnigOptionType options) {
        const unsigned char *unsigned_str = (unsigned char *)string.c_str();
        const unsigned char *char_end = unsigned_str + string.size();
        const unsigned char *char_start = unsigned_str + start;
        const unsigned char *char_range = char_end;
        return onig_search(m_regex, unsigned_str, char_end, char_start, char_range, region, options);
    }

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

    bool has_match(Env *env, Value, Value);
    Value initialize(Env *, Value, Value);
    Value inspect(Env *env);
    Value encoding(Env *env);
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

private:
    friend MatchDataObject;

    regex_t *m_regex { nullptr };
    int m_options { -1 };
    String m_pattern {};
};

}
