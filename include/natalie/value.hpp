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
        // This is required, because initialization by a literal is often ambiguous.
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
        auto_hydrate();
        return m_object;
    }

    Object *object_or_null() const {
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
        if (m_type == Type::Double) {
            if (other.m_type == Type::Double)
                return m_double == m_double;
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

    Value public_send(Env *, SymbolObject *, Args = {}, Block * = nullptr, Value sent_from = nullptr);

    Value public_send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr, Value sent_from = nullptr) {
        return public_send(env, name, Args(args), block, sent_from);
    }

    Value send(Env *, SymbolObject *, Args = {}, Block * = nullptr, Value sent_from = nullptr);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr, Value sent_from = nullptr) {
        return send(env, name, Args(args), block, sent_from);
    }

    bool is_fast_integer() const {
        return m_type == Type::Integer;
    }

    bool is_fast_double() const {
        return m_type == Type::Double;
    }

    double as_double() const;
    nat_int_t as_fast_integer() const;

    nat_int_t get_fast_integer() const {
        assert(m_type == Type::Integer);
        return m_integer;
    }

    double get_fast_double() const {
        assert(m_type == Type::Double);
        return m_double;
    }

private:
    explicit Value(double value)
        : m_type { Type::Double }
        , m_double { value } { }

    void auto_hydrate() {
        if (m_type != Type::Pointer)
            hydrate();
    }

    template <typename Callback>
    Value on_object_value(Callback &&callback);

    void hydrate();

    // Method can use a synthesized value if we are a fast double
    friend Method;
    bool holds_raw_double() const {
        return m_type == Type::Double;
    }

    Type m_type { Type::Pointer };

    union {
        nat_int_t m_integer { 0 };
        double m_double;
        Object *m_object;
    };
};

}
