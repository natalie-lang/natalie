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

namespace Natalie {

enum RegexOpts {
    IgnoreCase = 1,
    Extended = 2,
    MultiLine = 4
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

    static Value compile(Env *env, Value pattern, Value flags) {
        pattern->assert_type(env, Object::Type::String, "String");
        int options = 0;
        if (flags) {
            if (flags->is_integer())
                options = flags->as_integer()->to_nat_int_t();
            else if (flags->is_truthy())
                options = 1;
        }
        RegexpObject *regexp = new RegexpObject { env, pattern->as_string()->c_str(), options };
        return regexp;
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

    const char *pattern() { return m_pattern; }

    int options() const { return m_options; }

    void set_options(const String *options) {
        for (char *c = const_cast<char *>(options->c_str()); *c != '\0'; ++c) {
            switch (*c) {
            case 'i':
                m_options |= RegexOpts::IgnoreCase;
                break;
            case 'x':
                m_options |= RegexOpts::Extended;
                break;
            case 'm':
                m_options |= RegexOpts::MultiLine;
                break;
            default:
                break;
            }
        }
    }

    bool operator==(const Object &other) const {
        if (!other.is_regexp()) return false;
        RegexpObject *other_regexp = const_cast<Object &>(other).as_regexp();
        return strcmp(m_pattern, other_regexp->m_pattern) == 0 && m_options == other_regexp->m_options;
    }

    bool casefold() const {
        return m_options && RegexOpts::IgnoreCase;
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
        return *this == *other;
    }

    bool eqeqeq(Env *env, Value other) {
        if (!other->is_string() && !other->is_symbol()) {
            if (! other->respond_to(env, "to_str"_s))
                return false;
            other = other->send(env, "to_str"_s);
        }
        return match(env, other)->is_truthy();
    }

    Value initialize(Env *, Value);
    Value inspect(Env *env);
    Value eqtilde(Env *env, Value);
    Value match(Env *env, Value, size_t = 0);
    Value source(Env *env);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RegexpObject %p>", this);
    }

private:
    regex_t *m_regex { nullptr };
    int m_options { 0 };
    const char *m_pattern { nullptr };
};

}
