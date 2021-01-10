#pragma once

#include <assert.h>
#include <ctype.h>

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

    static ValuePtr to_proc_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);

    ValuePtr cmp(Env *, ValuePtr);

    bool is_constant() {
        return strlen(m_name) > 0 && isupper(m_name[0]);
    }

    bool start_with(Env *, ValuePtr);

    ValuePtr ref(Env *, ValuePtr);

private:
    SymbolValue(Env *env, const char *name)
        : Value { Value::Type::Symbol, env->Object()->const_fetch("Symbol")->as_class() }
        , m_name { GC_STRDUP(name) } {
        assert(m_name);
    }

    const char *m_name { nullptr };
};

}
