#pragma once

#include <initializer_list>
#include <iterator>

#include "natalie/forward.hpp"
#include "natalie/types.hpp"
#include "natalie/value_type.hpp"

namespace Natalie {

class ValuePtr {
public:
    enum class Type {
        Integer,
        Pointer,
    };

    ValuePtr() { }

    ValuePtr(Value *value)
        : m_value { value } { }

    ValuePtr(Type type, nat_int_t integer)
        : m_type { type }
        , m_integer { integer } { }

    static ValuePtr integer(nat_int_t integer) {
        return ValuePtr { Type::Integer, integer };
    }

    Value &operator*() {
        auto_hydrate();
        return *m_value;
    }

    Value *operator->() {
        auto_hydrate();
        return m_value;
    }

    Value *value() {
        auto_hydrate();
        return m_value;
    }

    Value *value_or_null() {
        if (m_type == Type::Pointer)
            return m_value;
        else
            return nullptr;
    }

    bool operator==(Value *other) {
        auto_hydrate();
        return m_value == other;
    }

    bool operator!=(Value *other) {
        auto_hydrate();
        return m_value != other;
    }

    bool operator!() { return m_type == Type::Pointer && !m_value; }

    operator bool() { return m_type == Type::Integer || m_value; }

    ValuePtr public_send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);

    ValuePtr send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);

    ValuePtr send(Env *env, SymbolValue *name, std::initializer_list<ValuePtr> args, Block *block = nullptr) {
        return send(env, name, args.size(), const_cast<ValuePtr *>(std::data(args)), block);
    }

    bool is_pointer() const {
        return m_type == Type::Pointer;
    }

    bool is_fast_integer() const {
        return m_type == Type::Integer;
    }

    bool is_integer();
    bool is_float();
    bool is_bignum();
    void assert_type(Env *, ValueType, const char *);
    nat_int_t to_nat_int_t();

    ValuePtr guard() {
        m_guarded = true;
        return *this;
    }

    ValuePtr unguard() {
        m_guarded = false;
        return *this;
    }

private:
    void auto_hydrate() {
        if (m_guarded) {
            printf("%p is a guarded ValuePtr, which means you must call unguard() on it before using the arrow operator.\n", this);
            abort();
        }
        if (m_type != Type::Pointer)
            hydrate();
    }

    void hydrate();

    bool m_guarded { false };

    Type m_type { Type::Pointer };

    nat_int_t m_integer { 0 };
    Value *m_value { nullptr };
};

}
