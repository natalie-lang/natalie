#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct MatchDataValue : Value {
    MatchDataValue(Env *env)
        : Value { Value::Type::MatchData, NAT_OBJECT->const_get(env, "MatchData", true)->as_class() } { }

    MatchDataValue(Env *env, ClassValue *klass)
        : Value { Value::Type::MatchData, klass } { }

    MatchDataValue(Env *env, OnigRegion *region, StringValue *string)
        : Value { Value::Type::MatchData, NAT_OBJECT->const_get(env, "MatchData", true)->as_class() }
        , m_region { region }
        , m_str { strdup(string->c_str()) } { }

    const char *c_str() { return m_str; }

    ssize_t size() { return m_region->num_regs; }

    ssize_t index(ssize_t);

    Value *group(Env *, ssize_t);

    Value *to_s(Env *);
    Value *ref(Env *, Value *);

private:
    OnigRegion *m_region { nullptr };
    const char *m_str { nullptr };
};
}
