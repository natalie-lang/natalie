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
        : Value { Value::Type::Array, env->Object()->const_get_or_panic(env, "Array", true)->as_class() } { }

    ArrayValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Array, klass } { }

    ArrayValue(Env *env, std::initializer_list<Value *> list)
        : ArrayValue { env } {
        m_vector.set_size(list.size());
        for (auto &v : list) {
            m_vector.push(v);
        }
    }

    ArrayValue(ArrayValue &other)
        : Value { other.type(), other.klass() }
        , m_vector { other.m_vector } { }

    // Array[]
    static Value *square_new(Env *env, ssize_t argc, Value **args) {
        ArrayValue *ary = new ArrayValue { env };
        for (ssize_t i = 0; i < argc; i++) {
            ary->push(args[i]);
        }
        return ary;
    }

    Value *to_ary() { return this; }

    ssize_t size() const { return m_vector.size(); }

    void push(Value &val) {
        m_vector.push(&val);
    }

    void push(Value *val) {
        m_vector.push(val);
    }

    Value *pop(Env *);

    Value *&operator[](ssize_t index) const {
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

    void sort_in_place(Env *);

    Vector<Value *>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<Value *>::iterator end() noexcept { return m_vector.end(); }

    Value *initialize(Env *, Value *, Value *);
    Value *inspect(Env *);
    Value *ltlt(Env *, Value *);
    Value *add(Env *, Value *);
    Value *sub(Env *, Value *);
    Value *ref(Env *, Value *, Value *);
    Value *refeq(Env *, Value *, Value *, Value *);
    Value *any(Env *, Block *);
    Value *eq(Env *, Value *);
    Value *each(Env *, Block *);
    Value *each_with_index(Env *, Block *);
    Value *map(Env *, Block *);
    Value *first(Env *);
    Value *last(Env *);
    Value *include(Env *, Value *);
    Value *sort(Env *);
    Value *join(Env *, Value *);
    Value *cmp(Env *, Value *);

private:
    Vector<Value *> m_vector {};
};

}
