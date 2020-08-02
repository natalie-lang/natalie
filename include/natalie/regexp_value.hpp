#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

extern "C" {
#include "onigmo.h"
}

namespace Natalie {

struct RegexpValue : Value {
    RegexpValue(Env *env)
        : Value { Value::Type::Regexp, env->Object()->const_get_or_panic(env, "Regexp", true)->as_class() } { }

    RegexpValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Regexp, klass } { }

    RegexpValue(Env *env, const char *pattern)
        : Value { Value::Type::Regexp, env->Object()->const_get_or_panic(env, "Regexp", true)->as_class() } {
        assert(pattern);
        initialize(env, pattern);
    }

    void initialize(Env *env, const char *pattern) {
        regex_t *regex;
        OnigErrorInfo einfo;
        UChar *pat = (UChar *)pattern;
        int result = onig_new(&regex, pat, pat + strlen((char *)pat),
            ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT, &einfo);
        if (result != ONIG_NORMAL) {
            OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
            onig_error_code_to_str(s, result, &einfo);
            NAT_RAISE(env, "SyntaxError", (char *)s);
        }
        m_regex = regex;
        m_pattern = strdup(pattern);
    }

    const char *pattern() { return m_pattern; }

    bool operator==(const Value &other) const {
        return other.is_regexp() && strcmp(m_pattern, const_cast<Value &>(other).as_regexp()->m_pattern) == 0;
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

    bool eq(Env *env, Value *other) {
        return *this == *other;
    }

    Value *initialize(Env *, Value *);
    Value *inspect(Env *env);
    Value *eqtilde(Env *env, Value *);
    Value *match(Env *env, Value *);

private:
    regex_t *m_regex { nullptr };
    const char *m_pattern { nullptr };
};

}
