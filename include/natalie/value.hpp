#pragma once

#include <assert.h>
#include <initializer_list>
#include <iterator>

#include "natalie/forward.hpp"
#include "natalie/object_type.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class Value {
public:
    enum class Type {
        Integer,
        Pointer,
    };

    Value() { }

    Value(Object *object)
        : m_object { object } { }

    Value(Type type, nat_int_t integer)
        : m_type { type }
        , m_integer { integer } { }

    static Value integer(nat_int_t integer) {
        return Value { Type::Integer, integer };
    }

    Object &operator*() {
        auto_hydrate();
        return *m_object;
    }

    Object *operator->() {
        auto_hydrate();
        return m_object;
    }

    Object *object() {
        auto_hydrate();
        return m_object;
    }

    Object *object_or_null() {
        if (m_type == Type::Pointer)
            return m_object;
        else
            return nullptr;
    }

    bool operator==(Object *other) {
        auto_hydrate();
        return m_object == other;
    }

    bool operator!=(Object *other) {
        auto_hydrate();
        return m_object != other;
    }

    bool operator!() { return m_type == Type::Pointer && !m_object; }

    operator bool() { return m_type == Type::Integer || m_object; }

    Value public_send(Env *, SymbolObject *, size_t = 0, Value * = nullptr, Block * = nullptr);

    Value send(Env *, SymbolObject *, size_t = 0, Value * = nullptr, Block * = nullptr);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr) {
        return send(env, name, args.size(), const_cast<Value *>(std::data(args)), block);
    }

    bool is_pointer() const {
        return m_type == Type::Pointer;
    }

    bool is_fast_integer() const {
        return m_type == Type::Integer;
    }

    nat_int_t get_fast_integer() const {
        assert(m_type == Type::Integer);
        return m_integer;
    }

    Value guard() {
        m_guarded = true;
        return *this;
    }

    Value unguard() {
        m_guarded = false;
        return *this;
    }

private:
    void auto_hydrate() {
        if (m_guarded) {
            printf("%p is a guarded Value, which means you must call unguard() on it before using the arrow operator.\n", this);
            abort();
        }
        if (m_type != Type::Pointer)
            hydrate();
    }

    void hydrate();

    bool m_guarded { false };

    Type m_type { Type::Pointer };

    union {
        nat_int_t m_integer { 0 };
        Object *m_object;
    };
};

}
