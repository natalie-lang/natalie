#pragma once

#include <assert.h>
#include <ctype.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {

class SymbolValue : public Value {
public:
    static SymbolValue *intern(const char *);
    static SymbolValue *intern(const String *);

    virtual ~SymbolValue() {
        free(const_cast<char *>(m_name));
    }

    const char *c_str() { return m_name; }

    StringValue *to_s(Env *env) { return new StringValue { m_name }; }
    StringValue *inspect(Env *);
    SymbolValue *succ(Env *);
    SymbolValue *upcase(Env *);

    virtual ProcValue *to_proc(Env *) override;

    static ValuePtr to_proc_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);

    ValuePtr cmp(Env *, ValuePtr);

    bool is_constant_name() {
        return strlen(m_name) > 0 && isupper(m_name[0]);
    }

    bool is_global_name() {
        return strlen(m_name) > 0 && m_name[0] == '$';
    }

    bool is_ivar_name() {
        return strlen(m_name) > 0 && m_name[0] == '@';
    }

    bool is_cvar_name() {
        return strlen(m_name) > 1 && m_name[0] == '@' && m_name[1] == '@';
    }

    bool start_with(Env *, ValuePtr);

    ValuePtr ref(Env *, ValuePtr);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<SymbolValue %p name='%s'>", this, m_name);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static TM::Hashmap<const char *, SymbolValue *> s_symbols { TM::HashType::String, 1000 };

    SymbolValue(Env *env, const char *name)
        : Value { Value::Type::Symbol, GlobalEnv::the()->Symbol() }
        , m_name { strdup(name) } {
        assert(m_name);
    }

    SymbolValue(const char *name)
        : Value { Value::Type::Symbol, GlobalEnv::the()->Symbol() }
        , m_name { strdup(name) } {
        assert(m_name);
    }

    const char *m_name { nullptr };
};

}
