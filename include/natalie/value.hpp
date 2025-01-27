#pragma once

#include <assert.h>
#include <initializer_list>
#include <iterator>

#include "natalie/args.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/integer.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/object_type.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class Value {
public:
    enum class Type {
        Integer,
        Pointer,
    };

    Value() = default;

    Value(Object *object)
        : m_type { Type::Pointer }
        , m_object { object } { }

    explicit Value(nat_int_t integer)
        : m_type { Type::Integer }
        , m_integer { integer } { }

    Value(const Integer &integer)
        : m_type { Type::Integer }
        , m_integer { integer } { }

    Value(Integer &&integer)
        : m_type { Type::Integer }
        , m_integer { std::move(integer) } { }

    static Value integer(nat_int_t integer) {
        // This is required, because initialization by a literal is often ambiguous.
        return Value { integer };
    }
    Type type() const { return m_type; }

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

    const Object *object_pointer() const {
        assert(m_type == Type::Pointer);
        return m_object;
    }

    bool operator==(Value other) const;

    bool operator!=(Value other) const {
        return !(*this == other);
    }

    bool is_null() const { return m_type == Type::Pointer && !m_object; }

    bool operator!() const { return is_null(); }

    operator bool() const { return !is_null(); }

    Value public_send(Env *, SymbolObject *, Args && = {}, Block * = nullptr, Value sent_from = nullptr);

    Value public_send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr, Value sent_from = nullptr) {
        return public_send(env, name, Args(args), block, sent_from);
    }

    Value send(Env *, SymbolObject *, Args && = {}, Block * = nullptr, Value sent_from = nullptr);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr, Value sent_from = nullptr) {
        return send(env, name, Args(args), block, sent_from);
    }

    Value integer_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from, MethodVisibility visibility);

    bool is_fast_integer() const {
        return m_type == Type::Integer;
    }

    nat_int_t as_fast_integer() const;

    nat_int_t get_fast_integer() const {
        assert(m_type == Type::Integer);
        return m_integer.to_nat_int_t();
    }

    const Integer &integer() const;
    Integer &integer();

    const Integer &integer_or_raise(Env *) const;
    Integer &integer_or_raise(Env *);

    bool is_integer() const;

    nat_int_t object_id() const;

    void assert_type(Env *, ObjectType, const char *) const;

private:
    void auto_hydrate() {
        if (m_type != Type::Pointer)
            hydrate();
    }

    template <typename Callback>
    Value on_object_value(Callback &&callback);

    void hydrate();

    Type m_type { Type::Pointer };

    union {
        Integer m_integer { 0 };
        Object *m_object;
    };
};

}
