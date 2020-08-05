#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct SymbolValue : Value {
    static SymbolValue *intern(Env *, const char *);

    const char *c_str() { return m_name; }

    StringValue *to_s(Env *env) { return new StringValue { env, m_name }; }
    StringValue *inspect(Env *);

    virtual ProcValue *to_proc(Env *) override;

    static Value *to_proc_block_fn(Env *, Value *, ssize_t, Value **, Block *);

    Value *cmp(Env *, Value *);

private:
    SymbolValue(Env *env, const char *name)
        : Value { Value::Type::Symbol, env->Object()->const_get_or_panic(env, "Symbol", true)->as_class() }
        , m_name { name } {
        assert(m_name);
    }

    const char *m_name { nullptr };
};

}
