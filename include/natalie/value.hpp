#pragma once

#include <assert.h>
#include <initializer_list>
#include <iterator>

#include "natalie/args.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/object_type.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class Value {
public:
    enum class Type {
        Integer,
        Double,
        Pointer,
    };

    Value() = default;

    Value(Object *object)
        : m_type { Type::Pointer }
        , m_object { object } { }

    explicit Value(nat_int_t integer)
        : m_type { Type::Integer }
        , m_integer { integer } { }

    static Value integer(nat_int_t integer) {
        // This is required, beacause initialization by a literal is often
        // ambiguous.
        return Value { integer };
    }
    static Value floatingpoint(double value);

    Object &operator*() {
        auto_hydrate();
        return *m_object;
    }

    Object *operator->() {
        auto_hydrate();
        return m_object;
    }

    Object *object() {
        if (m_type == Type::Pointer)
            return m_object;
        auto_hydrate();
        return m_object;
    }

    Object *object_or_null() {
        if (m_type == Type::Pointer)
            return m_object;
        else
            return nullptr;
    }

    bool operator==(Value other) const {
        if (is_fast_integer()) {
            if (other.is_fast_integer())
                return get_fast_integer() == other.get_fast_integer();
            return false;
        }
        if (is_fast_float()) {
            if (other.is_fast_float())
                return get_fast_float() == other.get_fast_float();
            return false;
        }
        return m_object == other.object();
    }

    bool operator!=(Value other) const {
        return !(*this == other);
    }

    bool is_null() const { return m_type == Type::Pointer && !m_object; }

    bool operator!() const { return is_null(); }

    operator bool() const { return !is_null(); }

    Value public_send(Env *, SymbolObject *, Args = {}, Block * = nullptr);

    Value public_send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr) {
        return public_send(env, name, Args(args), block);
    }

    Value send(Env *, SymbolObject *, Args = {}, Block * = nullptr);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr) {
        return send(env, name, Args(args), block);
    }

    bool is_pointer() const {
        return m_type == Type::Pointer;
    }

    bool is_fast_integer() const {
        return m_type == Type::Integer;
    }

    bool is_fast_float() const {
        return m_type == Type::Double;
    }

    nat_int_t get_fast_integer() const {
        assert(m_type == Type::Integer);
        return m_integer;
    }

    double get_fast_float() const {
        assert(m_type == Type::Double);
        return m_double;
    }

    bool guarded() const { return m_guarded; }

    Value guard() {
        m_guarded = true;
        return *this;
    }

    Value unguard() {
        m_guarded = false;
        return *this;
    }

private:
    explicit Value(double value)
        : m_type { Type::Double }
        , m_double { value } { }

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
        double m_double;
        Object *m_object;
    };
};

}
