#pragma once

#include <initializer_list>
#include <utility>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

class ArrayValue : public Value {
public:
    ArrayValue()
        : Value { Value::Type::Array, GlobalEnv::the()->Array() } { }

    ArrayValue(std::initializer_list<ValuePtr> list)
        : ArrayValue {} {
        m_vector.set_capacity(list.size());
        for (auto &v : list) {
            m_vector.push(v);
        }
    }

    ArrayValue(Env *env, ArrayValue &other)
        : Value { other }
        , m_vector { other.m_vector } { }

    ArrayValue(size_t argc, ValuePtr *args)
        : ArrayValue {} {
        for (size_t i = 0; i < argc; i++) {
            push(args[i]);
        }
    }

    // Array[]
    static ValuePtr square_new(Env *env, size_t argc, ValuePtr *args) {
        return new ArrayValue { argc, args };
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

    ValuePtr sort_in_place(Env *, Block *);
    ValuePtr sort_by_in_place(Env *, Block *);

    Vector<ValuePtr>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<ValuePtr>::iterator end() noexcept { return m_vector.end(); }

    bool is_empty() { return m_vector.is_empty(); }

    ValuePtr initialize(Env *, ValuePtr, ValuePtr, Block *);

    ValuePtr add(Env *, ValuePtr);
    ValuePtr any(Env *, size_t, ValuePtr *, Block *);
    ValuePtr at(Env *, ValuePtr);
    ValuePtr assoc(Env *, ValuePtr);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr clear(Env *);
    ValuePtr compact(Env *);
    ValuePtr concat(Env *, size_t, ValuePtr *);
    ValuePtr drop(Env *, ValuePtr);
    ValuePtr each(Env *, Block *);
    ValuePtr eq(Env *, ValuePtr);
    ValuePtr eql(Env *, ValuePtr);
    ValuePtr first(Env *, ValuePtr);
    ValuePtr flatten(Env *, ValuePtr);
    ValuePtr flatten_in_place(Env *, ValuePtr);
    ValuePtr include(Env *, ValuePtr);
    ValuePtr index(Env *, ValuePtr, Block *);
    ValuePtr inspect(Env *);
    ValuePtr intersection(Env *, ValuePtr);
    ValuePtr intersection(Env *, size_t, ValuePtr *);
    ValuePtr join(Env *, ValuePtr);
    ValuePtr last(Env *, ValuePtr);
    ValuePtr ltlt(Env *, ValuePtr);
    ValuePtr map(Env *, Block *);
    ValuePtr max(Env *);
    ValuePtr min(Env *);
    ValuePtr none(Env *, size_t, ValuePtr *, Block *);
    ValuePtr one(Env *, size_t, ValuePtr *, Block *);
    ValuePtr push(Env *, size_t, ValuePtr *);
    ValuePtr rassoc(Env *, ValuePtr);
    ValuePtr ref(Env *, ValuePtr, ValuePtr);
    ValuePtr refeq(Env *, ValuePtr, ValuePtr, ValuePtr);
    ValuePtr reject(Env *, Block *);
    ValuePtr reverse(Env *);
    ValuePtr reverse_in_place(Env *);
    ValuePtr rindex(Env *, ValuePtr, Block *);
    ValuePtr rotate(Env *, ValuePtr);
    ValuePtr rotate_in_place(Env *, ValuePtr);
    ValuePtr sample(Env *);
    ValuePtr select(Env *, Block *);
    ValuePtr shift(Env *, ValuePtr);
    ValuePtr slice_in_place(Env *, ValuePtr, ValuePtr);
    ValuePtr sort(Env *, Block *);
    ValuePtr sub(Env *, ValuePtr);
    ValuePtr union_of(Env *, ValuePtr);
    ValuePtr union_of(Env *, size_t, ValuePtr *);
    ValuePtr uniq_in_place(Env *, Block *);

    virtual void visit_children(Visitor &visitor) override final {
        Value::visit_children(visitor);
        for (auto val : m_vector) {
            visitor.visit(val);
        }
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        size_t size = m_vector.size();
        snprintf(buf, len, "<ArrayValue %p size=%zu>", this, size);
    }

private:
    ArrayValue(Vector<ValuePtr> &&vector)
        : Value { Value::Type::Array, GlobalEnv::the()->Array() }
        , m_vector { std::move(vector) } { }

    Vector<ValuePtr> m_vector {};

    bool _flatten_in_place(Env *, nat_int_t depth, Vector<ArrayValue *> visited_arrays);
};

}
