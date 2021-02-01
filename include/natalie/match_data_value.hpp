#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct MatchDataValue : Value {
    MatchDataValue(Env *env)
        : Value { Value::Type::MatchData, env->Object()->const_fetch(env, SymbolValue::intern(env, "MatchData"))->as_class() } { }

    MatchDataValue(Env *env, ClassValue *klass)
        : Value { Value::Type::MatchData, klass } { }

    MatchDataValue(Env *env, OnigRegion *region, StringValue *string)
        : Value { Value::Type::MatchData, env->Object()->const_fetch(env, SymbolValue::intern(env, "MatchData"))->as_class() }
        , m_region { region }
        , m_str { GC_STRDUP(string->c_str()) } { }

    const char *c_str() { return m_str; }

    size_t size() { return m_region->num_regs; }

    size_t index(size_t);

    ValuePtr group(Env *, size_t);

    ValuePtr to_s(Env *);
    ValuePtr ref(Env *, ValuePtr);

private:
    OnigRegion *m_region { nullptr };
    const char *m_str { nullptr };
};
}
