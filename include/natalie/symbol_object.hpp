#pragma once

#include <assert.h>
#include <ctype.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {

class SymbolObject : public Object {
public:
    enum class Ownedness {
        OwnedString,
        DuplicatedString,
    };

    static SymbolObject *intern(const char *, Ownedness ownedness = Ownedness::DuplicatedString);
    static SymbolObject *intern(const String *);

    virtual ~SymbolObject() {
        if (m_owned_string) free(const_cast<char *>(m_name));
    }

    const char *c_str() const { return m_name; }

    static ArrayObject *all_symbols(Env *);
    StringObject *to_s(Env *env) { return new StringObject { m_name }; }
    SymbolObject *to_sym(Env *env) { return this; }
    StringObject *inspect(Env *);
    SymbolObject *succ(Env *);
    SymbolObject *upcase(Env *);

    virtual ProcObject *to_proc(Env *) override;

    static Value to_proc_block_fn(Env *, Value, size_t, Value *, Block *);

    Value cmp(Env *, Value);

    bool is_constant_name() {
        return strlen(m_name) > 0 && isupper(m_name[0]);
    }

    bool is_global_name() {
        return strlen(m_name) > 0 && m_name[0] == '$';
    }

    bool is_ivar_name() {
        return strlen(m_name) > 1 && m_name[0] == '@' && isalpha(m_name[1]);
    }

    bool is_cvar_name() {
        return strlen(m_name) > 1 && m_name[0] == '@' && m_name[1] == '@';
    }

    bool is_empty() {
        return strlen(m_name) == 0;
    }

    bool start_with(Env *, Value);

    Value ref(Env *, Value);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<SymbolObject %p name='%s'>", this, m_name);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static TM::Hashmap<const char *, SymbolObject *> s_symbols { TM::HashType::String, 1000 };

    SymbolObject(const char *name, Ownedness ownedness)
        : Object { Object::Type::Symbol, GlobalEnv::the()->Symbol() }
        , m_owned_string { ownedness == Ownedness::OwnedString }
        , m_name { m_owned_string ? name : strdup(name) } {
        assert(m_name);
    }

    bool m_owned_string { false };
    const char *m_name { nullptr };
};

[[nodiscard]] __attribute__((always_inline)) inline SymbolObject *operator"" _s(const char *cstring, size_t) {
    return SymbolObject::intern(cstring, SymbolObject::Ownedness::OwnedString);
}

}
