#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/ctype.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {

class SymbolObject : public Object {
public:
    static SymbolObject *intern(const char *, size_t, EncodingObject * = nullptr);
    static SymbolObject *intern(const String &, EncodingObject * = nullptr);

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

    static Value to_proc_block_fn(Env *, Value, Args &&, Block *);

    Value cmp(Env *, Value);

    // Classification of a symbol name in the style of MRI's rb_enc_symname_type.
    // Computed once in the constructor and cached; symbols are immutable.
    enum class NameType : uint8_t {
        Local, // foo, _bar, föö
        Const, // Foo
        Global, // $foo, $0, $!, $-w
        IVar, // @foo
        CVar, // @@foo
        AttrSet, // foo=
        Junk, // foo?, foo!, and operator names (==, <<, []=, ...)
        Invalid, // anything that can't be written as a bare symbol literal
    };

    NameType name_type() const { return m_type; }

    bool is_local_name() const { return m_type == NameType::Local; }
    bool is_constant_name() const { return m_type == NameType::Const; }
    bool is_global_name() const { return m_type == NameType::Global; }
    bool is_ivar_name() const { return m_type == NameType::IVar; }
    bool is_cvar_name() const { return m_type == NameType::CVar; }
    bool is_invalid_name() const { return m_type == NameType::Invalid; }

    bool is_empty() {
        return m_name.is_empty();
    }

    bool end_with(Env *, Args &&);
    bool start_with(Env *, Args &&);

    Value eqtilde(Env *, Value);
    bool has_match(Env *, Value, Optional<Value> = {}) const;
    Value length(Env *);
    Value match(Env *, Value, Block *) const;
    Value name(Env *) const;
    Value ref(Env *, Value, Optional<Value> = {});

    const String &string() const { return m_name; }
    EncodingObject *encoding(Env *env) const { return m_encoding; }

    virtual String dbg_inspect(int indent = 0) const override;

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_string);
        visitor.visit(m_encoding);
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
    inline static TM::Hashmap<TM::String, SymbolObject *> s_symbols { 1000 };

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
        m_type = classify_name();
        freeze();
    }

    NameType classify_name() const;

    const TM::String m_name {};

    StringObject *m_string = nullptr;
    EncodingObject *m_encoding = nullptr;
    NameType m_type { NameType::Invalid };
};

[[nodiscard]] __attribute__((always_inline)) inline SymbolObject *operator""_s(const char *cstring, size_t length) {
    return SymbolObject::intern({ cstring, length });
}

}
