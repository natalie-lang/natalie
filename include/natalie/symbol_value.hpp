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
    static SymbolValue *from_id(Env *env, ID id) {
        SymbolValue *symbol = static_cast<SymbolValue *>(hashmap_get(env->global_env()->symbols(), (void *)id));
        if (!symbol) {
            printf("Tried to get a symbol with and ID (%zu) that was never intern'd!\n", id);
            abort();
        }
        return symbol;
    }

    static SymbolValue *intern(Env *env, const char *str) {
        ID id = hashmap_hash_string(str);
        return intern_and_return_symbol(env, id, str);
    }

    static SymbolValue *_new_only_for_use_by_intern(Env *env, ID id, const char *str) {
        return new SymbolValue { env, id, str };
    }

    const char *c_str() { return m_name; }

    StringValue *to_s(Env *env) { return new StringValue { env, m_name }; }
    StringValue *inspect(Env *);

    virtual ProcValue *to_proc(Env *) override;

    static Value *to_proc_block_fn(Env *, Value *, ssize_t, Value **, Block *);

    Value *cmp(Env *, Value *);

    ID id() { return m_id; }

private:
    SymbolValue(Env *env, ID id, const char *name)
        : Value { Value::Type::Symbol, env->Object()->const_get_or_panic(env, "Symbol", true)->as_class() }
        , m_name { name }
        , m_id { id } {
        assert(m_name);
    }

    const char *m_name { nullptr };
    ID m_id { 0 };
};

}
