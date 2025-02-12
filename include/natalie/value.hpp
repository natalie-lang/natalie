#pragma once

#include <assert.h>
#include <initializer_list>
#include <iterator>

#include "natalie/args.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/object_type.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class Value {
public:
    Value() = default;

    Value(Object *object)
        : m_value { reinterpret_cast<uintptr_t>(object) } { }

    explicit Value(nat_int_t integer) {
        m_value = left_shift_with_undefined_behavior(integer, 1) | 0x1;
    }

    Value(const Integer integer);

    static Value integer(nat_int_t integer) {
        // This is required, because initialization by a literal is often ambiguous.
        return Value { integer };
    }

    static Value integer(TM::String &&str);

    Object &operator*() const {
        if (is_integer()) {
            fprintf(stderr, "Fatal: cannot dereference Value of type Integer\n");
            abort();
        }

        return *reinterpret_cast<Object *>(m_value);
    }

    Object *operator->() const {
        if (is_integer()) {
            fprintf(stderr, "Fatal: cannot dereference Value of type Integer\n");
            abort();
        }

        return reinterpret_cast<Object *>(m_value);
    }

    Object *object() const {
        if (is_integer()) {
            fprintf(stderr, "Fatal: cannot dereference Value of type Integer\n");
            abort();
        }

        return reinterpret_cast<Object *>(m_value);
    }

    bool is_null() const { return m_value == 0x0; }

    bool operator==(Value other) const { return m_value == other.m_value; }
    bool operator!=(Value other) const { return m_value != other.m_value; }

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

    ClassObject *klass() const;
    ClassObject *singleton_class() const;
    ClassObject *singleton_class(Env *);

    // Old error message style, e.g.:
    // - no implicit conversion from nil to string
    // - no implicit conversion of Integer into String
    StringObject *to_str(Env *env);

    // New error message style, e.g.:
    // - no implicit conversion of nil into String
    // - no implicit conversion of Integer into String
    StringObject *to_str2(Env *env);

    Integer integer() const;
    Integer integer_or_raise(Env *) const;

    bool is_pointer() const { return (m_value & 0x1) == 0x0; }
    bool is_fixnum() const { return (m_value & 0x1) == 0x1; }
    bool is_integer() const;

    nat_int_t object_id() const { return (nat_int_t)m_value; }

    void assert_integer(Env *) const;
    void assert_type(Env *, ObjectType, const char *) const;
    void assert_not_frozen(Env *) const;

    bool is_a(Env *, Value) const;
    bool respond_to(Env *, SymbolObject *, bool include_all = true);

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

    ArrayObject *to_ary(Env *env);
    FloatObject *to_f(Env *env);
    HashObject *to_hash(Env *env);
    IoObject *to_io(Env *env);
    Integer to_int(Env *env);
    StringObject *to_s(Env *env);

    enum class Conversion {
        Strict,
        NullAllowed,
    };

    SymbolObject *to_symbol(Env *, Conversion);

    String inspect_str(Env *);
    String dbg_inspect() const;

private:
    friend MarkingVisitor;

    Object *pointer() const {
        if (is_fixnum()) {
            fprintf(stderr, "Fatal: cannot dereference Value of type Fixnum\n");
            abort();
        }

        return reinterpret_cast<Object *>(m_value);
    }

    __attribute__((no_sanitize("undefined"))) static nat_int_t left_shift_with_undefined_behavior(nat_int_t x, nat_int_t y) {
        return x << y; // NOLINT
    }

    // The least significant bit is used to tag the pointer as either
    // an immediate value (63 bits) or a pointer to an Object.
    // If bit is 1, then shift the value to the right to get the actual
    // 63-bit number. If the bit is 0, then treat the value as a pointer.
    uintptr_t m_value { 0x0 };
};

}
