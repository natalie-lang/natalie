#pragma once

#include <initializer_list>
#include <utility>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "natalie/vector.hpp"

namespace Natalie {

struct ArrayValue : Value {
    ArrayValue(Env *env)
        : Value { Value::Type::Array, env->Array() } { }

    ArrayValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Array, klass } { }

    ArrayValue(Env *env, std::initializer_list<ValuePtr> list)
        : ArrayValue { env } {
        m_vector.set_capacity(list.size());
        for (auto &v : list) {
            m_vector.push(v);
        }
    }

    ArrayValue(ArrayValue &other)
        : Value { other }
        , m_vector { other.m_vector } { }

    ArrayValue(Env *env, size_t argc, ValuePtr *args)
        : ArrayValue { env } {
        for (size_t i = 0; i < argc; i++) {
            push(args[i]);
        }
    }

    // Array[]
    static ValuePtr square_new(Env *env, size_t argc, ValuePtr *args) {
        return new ArrayValue { env, argc, args };
    }

    ValuePtr to_ary() { return this; }

    size_t size() const { return m_vector.size(); }

    void push(Value &val) {
        m_vector.push(&val);
    }

    void push(ValuePtr val) {
        m_vector.push(val);
    }

    ValuePtr pop(Env *);

    ValuePtr &operator[](size_t index) const {
        assert(index < m_vector.size());
        return m_vector[index];
    }

    void concat(ArrayValue &other) {
        for (ValuePtr v : other) {
            push(v);
        }
    }

    void push_splat(Env *, ValuePtr);

    void expand_with_nil(Env *, size_t);

    void overwrite(ArrayValue &other) {
        m_vector.set_size(0);
        for (ValuePtr v : other) {
            push(v);
        }
    }

    ValuePtr *data() {
        return m_vector.data();
    }

    void sort_in_place(Env *);

    Vector<ValuePtr>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<ValuePtr>::iterator end() noexcept { return m_vector.end(); }

    bool is_empty() { return m_vector.is_empty(); }

    ValuePtr initialize(Env *, ValuePtr, ValuePtr);

    ValuePtr add(Env *, ValuePtr);
    ValuePtr any(Env *, size_t, ValuePtr *, Block *);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr compact(Env *);
    ValuePtr each(Env *, Block *);
    ValuePtr eq(Env *, ValuePtr);
    ValuePtr eql(Env *, ValuePtr);
    ValuePtr first(Env *);
    ValuePtr include(Env *, ValuePtr);
    ValuePtr index(Env *, ValuePtr, Block *);
    ValuePtr inspect(Env *);
    ValuePtr join(Env *, ValuePtr);
    ValuePtr last(Env *);
    ValuePtr ltlt(Env *, ValuePtr);
    ValuePtr map(Env *, Block *);
    ValuePtr max(Env *);
    ValuePtr min(Env *);
    ValuePtr push(Env *, size_t, ValuePtr *);
    ValuePtr ref(Env *, ValuePtr, ValuePtr);
    ValuePtr refeq(Env *, ValuePtr, ValuePtr, ValuePtr);
    ValuePtr reject(Env *, Block *);
    ValuePtr sample(Env *);
    ValuePtr select(Env *, Block *);
    ValuePtr shift(Env *, ValuePtr);
    ValuePtr sort(Env *);
    ValuePtr sub(Env *, ValuePtr);
    ValuePtr uniq(Env *);

private:
    ArrayValue(Env *env, Vector<ValuePtr> &&vector)
        : Value { Value::Type::Array, env->Array() }
        , m_vector { std::move(vector) } { }

    Vector<ValuePtr> m_vector {};
};

}
