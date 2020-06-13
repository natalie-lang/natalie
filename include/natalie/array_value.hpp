#pragma once

#include <initializer_list>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "natalie/vector.hpp"

namespace Natalie {

struct ArrayValue : Value {
    ArrayValue(Env *env)
        : Value { Value::Type::Array, NAT_OBJECT->const_get(env, "Array", true)->as_class() } { }

    ArrayValue(Env *env, std::initializer_list<Value *> list)
        : ArrayValue { env } {
        m_vector.set_size(list.size());
        for (auto &v : list) {
            m_vector.push(v);
        }
    }

    ArrayValue(const ArrayValue &other)
        : Value { Value::Type::Array, other.klass }
        , m_vector { other.m_vector } { }

    // TODO: remove this
    static ArrayValue *copy(Env *env, ArrayValue &other) {
        ArrayValue *array = new ArrayValue { env };
        array->overwrite(other);
        return array;
    }

    ssize_t size() { return m_vector.size(); }

    void push(Value &val) {
        m_vector.push(&val);
    }

    void push(Value *val) {
        m_vector.push(val);
    }

    Value *pop(Env *);

    Value *&operator[](ssize_t index) {
        return m_vector[index];
    }

    void concat(ArrayValue &other) {
        for (Value *v : other) {
            push(v);
        }
    }

    void push_splat(Env *, Value *);

    void expand_with_nil(Env *, ssize_t);

    void overwrite(ArrayValue &other) {
        m_vector.set_size(0);
        for (Value *v : other) {
            push(v);
        }
    }

    Value **data() {
        return m_vector.data();
    }

    void sort(Env *);

    Vector<Value *>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<Value *>::iterator end() noexcept { return m_vector.end(); }

    Vector<Value *> m_vector {};

private:
};

}
