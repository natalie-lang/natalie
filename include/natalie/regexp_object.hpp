#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"

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

    RegexpObject(Env *env, const char *pattern, int options = 0)
        : Object { Object::Type::Regexp, GlobalEnv::the()->Regexp() } {
        assert(pattern);
        initialize(env, pattern, options);
    }

    virtual ~RegexpObject() {
        onig_free(m_regex);
        free(const_cast<char *>(m_pattern));
    }

    static Value compile(Env *env, Value pattern, Value flags, ClassObject *klass) {
        pattern->assert_type(env, Object::Type::String, "String");
        RegexpObject *regexp = new RegexpObject { klass };
        regexp->send(env, "initialize"_s, { pattern, flags });
        return regexp;
    }

    static Value last_match(Env *, Value);

    static Value literal(Env *env, const char *pattern, int options = 0) {
        auto regex = new RegexpObject(env, pattern, options);
        regex->freeze();
        return regex;
    }

    Value hash(Env *env) {
        assert_initialized(env);
        auto hash = (m_options & ~RegexOpts::NoEncoding) + TM::Hashmap<void *>::hash_str(m_pattern);
        return Value::integer(hash);
    }

    void initialize(Env *env, const char *pattern, int options = 0) {
        regex_t *regex;
        OnigErrorInfo einfo;
        m_pattern = strdup(pattern);
        UChar *pat = (UChar *)m_pattern;
        int result = onig_new(&regex, pat, pat + strlen(m_pattern),
            options, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT, &einfo);
        if (result != ONIG_NORMAL) {
            OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
            onig_error_code_to_str(s, result, &einfo);
            env->raise("SyntaxError", (char *)s);
        }
        m_regex = regex;
        m_options = options;
    }

    bool is_initialized() const { return m_pattern != nullptr; }

    void assert_initialized(Env *env) const {
        if (!is_initialized())
            env->raise("TypeError", "uninitialized Regexp");
    }

    const char *pattern() { return m_pattern; }

    int options() const { return m_options; }

    int options(Env *env) {
        assert_initialized(env);
        return m_options;
    }

    void set_options(const String *options) {
        parse_options(options, &m_options);
    }

    static void parse_options(const String *options_string, int *options) {
        for (char *c = const_cast<char *>(options_string->c_str()); *c != '\0'; ++c) {
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
        return strcmp(m_pattern, other.m_pattern) == 0 && (m_options | RegexOpts::NoEncoding) == (other.m_options | RegexOpts::NoEncoding);
        // /n encoding option is ignored when doing == in ruby MRI
    }

    bool casefold(Env *env) const {
        assert_initialized(env);
        return m_options & RegexOpts::IgnoreCase;
    }

    int search(const char *str, OnigRegion *region, OnigOptionType options) {
        return search(str, 0, region, options);
    }

    int search(const char *str, int start, OnigRegion *region, OnigOptionType options) {
        unsigned char *unsigned_str = (unsigned char *)str;
        unsigned char *char_end = unsigned_str + strlen(str);
        unsigned char *char_start = unsigned_str + start;
        unsigned char *char_range = char_end;
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
            other = other->send(env, "to_str"_s);
        }
        return match(env, other)->is_truthy();
    }

    Value tilde(Env *env) {
        return this->send(env, "=~"_s, { env->global_get("$_"_s) });
    }

    bool has_match(Env *env, Value, Value);
    Value initialize(Env *, Value, Value);
    Value inspect(Env *env);
    Value eqtilde(Env *env, Value);
    Value match(Env *env, Value, Value = nullptr, Block * = nullptr);
    Value source(Env *env);
    Value to_s(Env *env);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RegexpObject %p>", this);
    }

private:
    regex_t *m_regex { nullptr };
    int m_options { 0 };
    const char *m_pattern { nullptr };
};

}
