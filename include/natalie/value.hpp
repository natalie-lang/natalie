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

    bool is_nil() const;
    bool is_true() const;
    bool is_false() const;
    bool is_fiber() const;
    bool is_enumerator_arithmetic_sequence() const;
    bool is_array() const;
    bool is_binding() const;
    bool is_method() const;
    bool is_module() const;
    bool is_class() const;
    bool is_complex() const;
    bool is_dir() const;
    bool is_encoding() const;
    bool is_env() const;
    bool is_exception() const;
    bool is_float() const;
    bool is_hash() const;
    bool is_io() const;
    bool is_file() const;
    bool is_file_stat() const;
    bool is_match_data() const;
    bool is_proc() const;
    bool is_random() const;
    bool is_range() const;
    bool is_rational() const;
    bool is_regexp() const;
    bool is_symbol() const;
    bool is_string() const;
    bool is_thread() const;
    bool is_thread_backtrace_location() const;
    bool is_thread_group() const;
    bool is_thread_mutex() const;
    bool is_time() const;
    bool is_unbound_method() const;
    bool is_void_p() const;

    bool is_truthy() const;
    bool is_falsey() const;
    bool is_numeric() const;
    bool is_boolean() const;

private:
    void auto_hydrate();

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
