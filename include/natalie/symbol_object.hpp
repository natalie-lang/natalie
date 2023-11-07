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
    static SymbolObject *intern(const char *, const size_t length, EncodingObject *encoding = nullptr);
    static SymbolObject *intern(const String &, EncodingObject *encoding = nullptr);

    static ArrayObject *all_symbols(Env *);
    StringObject *to_s(Env *);
    SymbolObject *to_sym(Env *env) { return this; }
    StringObject *inspect(Env *);
    SymbolObject *succ(Env *);
    SymbolObject *upcase(Env *);
    SymbolObject *downcase(Env *);
    SymbolObject *swapcase(Env *);
    SymbolObject *capitalize(Env *);
    Value casecmp(Env *, Value);
    Value is_casecmp(Env *, Value);

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
        return m_name.length() > 2 && m_name[0] == '@' && m_name[1] == '@';
    }

    bool is_empty() {
        return m_name.is_empty();
    }

    bool end_with(Env *, Args);
    bool start_with(Env *, Args);

    Value eqtilde(Env *, Value);
    bool has_match(Env *, Value, Value = nullptr) const;
    Value length(Env *);
    Value match(Env *, Value, Block *) const;
    Value name(Env *) const;
    Value ref(Env *, Value, Value);

    const String &string() const { return m_name; }
    EncodingObject *encoding(Env *env) const { return m_encoding; }

    bool should_be_quoted() const;

    virtual String dbg_inspect() const override;

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_string);
        visitor.visit(m_encoding);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        // NOTE: this won't properly print the null character '\0', but since this is only used
        // for debugging, we probably don't care.
        snprintf(buf, len, "<SymbolObject %p name='%s'>", this, m_name.c_str());
    }

    virtual bool is_collectible() override {
        return false;
    }

    static void visit_all_symbols(Visitor &visitor) {
        for (auto pair : s_symbols) {
            visitor.visit(pair.second);
        }
    }

private:
    inline static TM::Hashmap<const TM::String, SymbolObject *> s_symbols { TM::HashType::TMString, 1000 };
    inline static regex_t *s_inspect_quote_regex { nullptr };

    SymbolObject(const String &name, EncodingObject *encoding)
        : Object { Object::Type::Symbol, GlobalEnv::the()->Symbol() }
        , m_name { name }
        , m_encoding { encoding } {
        if (m_encoding == nullptr) m_encoding = EncodingObject::default_internal();
        if (m_encoding == nullptr) m_encoding = EncodingObject::default_external();

        if (m_encoding != nullptr && m_encoding->is_ascii_compatible()) {
            auto us_ascii = EncodingObject::get(Encoding::US_ASCII);
            bool all_ascii = true;
            for (size_t i = 0; all_ascii && i < name.length(); i++) {
                if (!us_ascii->in_encoding_codepoint_range(name[i])) {
                    all_ascii = false;
                    break;
                }
            }
            if (all_ascii) m_encoding = EncodingObject::get(Encoding::US_ASCII);
        }
        freeze();
    }

    const TM::String m_name {};

    StringObject *m_string = nullptr;
    EncodingObject *m_encoding = nullptr;
};

[[nodiscard]] __attribute__((always_inline)) inline SymbolObject *operator"" _s(const char *cstring, size_t) {
    return SymbolObject::intern(cstring);
}

}
