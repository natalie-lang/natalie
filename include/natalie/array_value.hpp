#pragma once

#include <iterator>
#include <vector>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "natalie/vector_temporary_delete_this_soon.hpp"

namespace Natalie {

struct ArrayValue : Value {
    ArrayValue(Env *env)
        : Value { env, Value::Type::Array, NAT_OBJECT->const_get(env, "Array", true)->as_class() } { }

    ArrayValue(Env *env, std::initializer_list<Value *> list)
        : ArrayValue { env } {
        for (auto &v : list) {
            m_vector.push_back(v);
        }
    }

    static ArrayValue *copy(Env *env, ArrayValue &other) {
        ArrayValue *array = new ArrayValue { env };
        array->overwrite(other);
        return array;
    }

    ssize_t size() { return m_vector.size(); }

    void push(Value &val) {
        m_vector.push_back(&val);
    }

    void push(Value *val) {
        m_vector.push_back(val);
    }

    Value *pop(Env *);

    Value *&operator[](ssize_t index) {
        return m_vector.at(index);
    }

    void concat(ArrayValue &other) {
        for (auto &v : other) {
            push(*v);
        }
    }

    void push_splat(Env *, Value *);

    void expand_with_nil(Env *, ssize_t);

    void overwrite(ArrayValue &other) {
        m_vector.clear();
        for (auto &v : other) {
            push(*v);
        }
    }

    Value **data() {
        return m_vector.data();
    }

    void sort(Env *);

    std::vector<Value *>::iterator begin() noexcept { return m_vector.begin(); }
    std::vector<Value *>::iterator end() noexcept { return m_vector.end(); }

private:
    std::vector<Value *> m_vector;
};

}
