#pragma once

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
        hydrate();
        return *m_value;
    }

    Value *operator->() {
        hydrate();
        return m_value;
    }

    Value *value() {
        hydrate();
        return m_value;
    }

    Value *value_or_null() {
        if (m_type == Type::Pointer)
            return m_value;
        else
            return nullptr;
    }

    bool operator==(Value *other) {
        hydrate();
        return m_value == other;
    }

    bool operator!=(Value *other) {
        hydrate();
        return m_value != other;
    }

    bool operator!() { return m_type == Type::Pointer && !m_value; }

    operator bool() { return m_type == Type::Integer || m_value; }

    ValuePtr public_send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);

    ValuePtr send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);
    ValuePtr send(Env *, const char *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);

    bool is_pointer() {
        return m_type == Type::Pointer;
    }

    bool is_integer();
    bool is_float();
    void assert_type(Env *, ValueType, const char *);
    nat_int_t to_nat_int_t();

private:
    void hydrate();

    Type m_type { Type::Pointer };
    nat_int_t m_integer { 0 };
    Value *m_value { nullptr };
};

}
