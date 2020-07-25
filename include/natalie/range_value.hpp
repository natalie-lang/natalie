#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct RangeValue : Value {
    RangeValue(Env *env)
        : Value { Value::Type::Range, env->Object()->const_get(env, "Range", true)->as_class() } { }

    RangeValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Range, klass } { }

    RangeValue(Env *env, Value *begin, Value *end, bool exclude_end)
        : Value { Value::Type::Range, env->Object()->const_get(env, "Range", true)->as_class() }
        , m_begin { begin }
        , m_end { end }
        , m_exclude_end { exclude_end } { }

    Value *begin() { return m_begin; }
    Value *end() { return m_end; }
    bool exclude_end() { return m_exclude_end; }

    Value *initialize(Env *, Value *, Value *, Value *);
    Value *to_a(Env *);
    Value *each(Env *, Block *);
    Value *inspect(Env *);
    Value *eq(Env *, Value *);
    Value *eqeqeq(Env *, Value *);

private:
    Value *m_begin { nullptr };
    Value *m_end { nullptr };
    bool m_exclude_end { false };
};

}
