#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"

extern "C" {
#include "onigmo.h"
}

namespace Natalie {

class RegexpValue : public Value {
public:
    RegexpValue()
        : Value { Value::Type::Regexp, GlobalEnv::the()->Regexp() } { }

    RegexpValue(ClassValue *klass)
        : Value { Value::Type::Regexp, klass } { }

    RegexpValue(Env *env, const char *pattern, int options = 0)
        : Value { Value::Type::Regexp, GlobalEnv::the()->Regexp() } {
        assert(pattern);
        initialize(env, pattern, options);
    }

    virtual ~RegexpValue() {
        onig_free(m_regex);
        free(const_cast<char *>(m_pattern));
    }

    static ValuePtr compile(Env *env, ValuePtr pattern, ValuePtr flags) {
        pattern->assert_type(env, Value::Type::String, "String");
        int options = 0;
        if (flags) {
            if (flags->is_integer())
                options = flags->as_integer()->to_nat_int_t();
            else if (flags->is_truthy())
                options = 1;
        }
        RegexpValue *regexp = new RegexpValue { env, pattern->as_string()->c_str(), options };
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
    int options() { return m_options; }

    bool operator==(const Value &other) const {
        if (!other.is_regexp()) return false;
        RegexpValue *other_regexp = const_cast<Value &>(other).as_regexp();
        return strcmp(m_pattern, other_regexp->m_pattern) == 0 && m_options == other_regexp->m_options;
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

    bool eq(Env *env, ValuePtr other) {
        return *this == *other;
    }

    bool eqeqeq(Env *env, ValuePtr other) {
        if (!other->is_string() && !other->is_symbol()) {
            return false;
        }
        return match(env, other)->is_truthy();
    }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr inspect(Env *env);
    ValuePtr eqtilde(Env *env, ValuePtr);
    ValuePtr match(Env *env, ValuePtr, size_t = 0);
    ValuePtr source(Env *env);

    virtual void gc_print() override {
        fprintf(stderr, "<RegexpValue %p>", this);
    }

private:
    regex_t *m_regex { nullptr };
    int m_options { 0 };
    const char *m_pattern { nullptr };
};

}
