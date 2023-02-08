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
    static SymbolObject *intern(const char *, size_t length);
    static SymbolObject *intern(const String &);

    static ArrayObject *all_symbols(Env *);
    StringObject *to_s(Env *env) { return new StringObject { m_name }; }
    SymbolObject *to_sym(Env *env) { return this; }
    StringObject *inspect(Env *);
    SymbolObject *succ(Env *);
    SymbolObject *upcase(Env *);

    virtual ProcObject *to_proc(Env *) override;

    static Value to_proc_block_fn(Env *, Value, Args, Block *);

    Value cmp(Env *, Value);

    bool is_constant_name() {
        return m_name.length() > 0 && isupper(m_name[0]);
    }

    bool is_global_name() {
        return m_name.length() > 0 && m_name[0] == '$';
    }

    bool is_ivar_name() {
        return m_name.length() > 1 && m_name[0] == '@' && (isalpha(m_name[1]) || m_name[1] == '_');
    }

    bool is_cvar_name() {
        return m_name.length() > 1 && m_name[0] == '@' && m_name[1] == '@';
    }

    bool is_empty() {
        return m_name.is_empty();
    }

    bool start_with(Env *, Value);

    Value eqtilde(Env *, Value);
    Value length(Env *);
    Value name(Env *) const;
    Value ref(Env *, Value);

    const String &string() const { return m_name; }

    virtual String dbg_inspect() const override;

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_string);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        // NOTE: this won't properly print the null character '\0', but since this is only used
        // for debugging, we probably don't care.
        snprintf(buf, len, "<SymbolObject %p name='%s'>", this, m_name.c_str());
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static TM::Hashmap<const TM::String, SymbolObject *> s_symbols { TM::HashType::TMString, 1000 };

    SymbolObject(const String &name)
        : Object { Object::Type::Symbol, GlobalEnv::the()->Symbol() }
        , m_name { name } { }

    const TM::String m_name {};

    StringObject *m_string = nullptr;
};

[[nodiscard]] __attribute__((always_inline)) inline SymbolObject *operator"" _s(const char *cstring, size_t) {
    return SymbolObject::intern(cstring);
}

}
