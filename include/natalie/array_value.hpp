#pragma once

#include <initializer_list>
#include <utility>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "tm/recursion_guard.hpp"
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

    ArrayValue(ArrayValue &other)
        : Value { other }
        , m_vector { other.m_vector } { }

    ArrayValue &operator=(ArrayValue &&other) {
        Value::operator=(std::move(other));
        m_vector = std::move(other.m_vector);
        return *this;
    }

    ArrayValue(size_t argc, ValuePtr *args)
        : ArrayValue { argc, args, GlobalEnv::the()->Array() } { }

    ArrayValue(size_t argc, ValuePtr *args, ClassValue *klass)
        : Value { Value::Type::Array, klass } {
        for (size_t i = 0; i < argc; i++) {
            push(args[i]);
        }
    }

    // Array[]
    static ValuePtr square_new(Env *env, size_t argc, ValuePtr *args, ClassValue *klass) {
        return new ArrayValue { argc, args, klass };
    }

    static ValuePtr allocate(Env *, size_t, ValuePtr *);

    ValuePtr to_ary_method() { return this; }

    size_t size() const { return m_vector.size(); }

    void push(Value &val) {
        m_vector.push(&val);
    }

    void push(ValuePtr val) {
        m_vector.push(val);
    }

    ValuePtr pop(Env *, ValuePtr);

    ValuePtr &at(size_t index) const {
        assert(index < m_vector.size());
        return m_vector[index];
    }

    ValuePtr &operator[](size_t index) const {
        assert(index < m_vector.size()); // TODO: remove this assertion (audit what's using this operator first though)
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
    ValuePtr bsearch(Env *, Block *);
    ValuePtr bsearch_index(Env *, Block *);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr clear(Env *);
    ValuePtr compact(Env *);
    ValuePtr compact_in_place(Env *);
    ValuePtr concat(Env *, size_t, ValuePtr *);
    ValuePtr cycle(Env *, ValuePtr, Block *);
    ValuePtr delete_at(Env *, ValuePtr);
    ValuePtr delete_if(Env *, Block *);
    ValuePtr delete_item(Env *, ValuePtr, Block *);
    ValuePtr difference(Env *, size_t, ValuePtr *);
    ValuePtr dig(Env *, size_t, ValuePtr *);
    ValuePtr drop(Env *, ValuePtr);
    ValuePtr drop_while(Env *, Block *);
    ValuePtr each(Env *, Block *);
    ValuePtr each_index(Env *, Block *);
    ValuePtr eq(Env *, ValuePtr);
    ValuePtr eql(Env *, ValuePtr);
    ValuePtr fetch(Env *, ValuePtr, ValuePtr, Block *);
    ValuePtr fill(Env *, ValuePtr, ValuePtr, ValuePtr, Block *);
    ValuePtr first(Env *, ValuePtr);
    ValuePtr flatten(Env *, ValuePtr);
    ValuePtr flatten_in_place(Env *, ValuePtr);
    ValuePtr hash(Env *);
    ValuePtr include(Env *, ValuePtr);
    ValuePtr index(Env *, ValuePtr, Block *);
    ValuePtr initialize_copy(Env *, ValuePtr);
    ValuePtr inspect(Env *);
    ValuePtr insert(Env *, size_t, ValuePtr *);
    ValuePtr intersection(Env *, ValuePtr);
    ValuePtr intersection(Env *, size_t, ValuePtr *);
    ValuePtr join(Env *, ValuePtr);
    ValuePtr keep_if(Env *, Block *);
    ValuePtr last(Env *, ValuePtr);
    ValuePtr ltlt(Env *, ValuePtr);
    ValuePtr map(Env *, Block *);
    ValuePtr map_in_place(Env *, Block *);
    ValuePtr max(Env *, ValuePtr, Block *);
    ValuePtr min(Env *, ValuePtr, Block *);
    ValuePtr minmax(Env *, Block *);
    ValuePtr multiply(Env *, ValuePtr);
    ValuePtr none(Env *, size_t, ValuePtr *, Block *);
    ValuePtr one(Env *, size_t, ValuePtr *, Block *);
    ValuePtr pack(Env *, ValuePtr);
    ValuePtr product(Env *, size_t, ValuePtr *, Block *);
    ValuePtr push(Env *, size_t, ValuePtr *);
    ValuePtr rassoc(Env *, ValuePtr);
    ValuePtr ref(Env *, ValuePtr, ValuePtr = nullptr);
    ValuePtr refeq(Env *, ValuePtr, ValuePtr, ValuePtr);
    ValuePtr reject(Env *, Block *);
    ValuePtr reject_in_place(Env *, Block *);
    ValuePtr reverse(Env *);
    ValuePtr reverse_each(Env *, Block *);
    ValuePtr reverse_in_place(Env *);
    ValuePtr rindex(Env *, ValuePtr, Block *);
    ValuePtr rotate(Env *, ValuePtr);
    ValuePtr rotate_in_place(Env *, ValuePtr);
    ValuePtr select(Env *, Block *);
    ValuePtr select_in_place(Env *, Block *);
    ValuePtr shift(Env *, ValuePtr);
    ValuePtr slice_in_place(Env *, ValuePtr, ValuePtr);
    ValuePtr sort(Env *, Block *);
    ValuePtr sub(Env *, ValuePtr);
    static ValuePtr try_convert(Env *, ValuePtr);
    ValuePtr sum(Env *, size_t, ValuePtr *, Block *);
    ValuePtr union_of(Env *, ValuePtr);
    ValuePtr union_of(Env *, size_t, ValuePtr *);
    ValuePtr uniq_in_place(Env *, Block *);
    ValuePtr unshift(Env *, size_t, ValuePtr *);
    ValuePtr to_h(Env *, Block *);
    ValuePtr values_at(Env *, size_t, ValuePtr *);
    ValuePtr zip(Env *, size_t, ValuePtr *, Block *);

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

    nat_int_t _resolve_index(nat_int_t);
    bool _flatten_in_place(Env *, nat_int_t depth, Hashmap<ArrayValue *> visited_arrays = Hashmap<ArrayValue *> {});
    ValuePtr _slice_in_place(nat_int_t start, nat_int_t end, bool exclude_end);
    ValuePtr find_index(Env *, ValuePtr, Block *, bool = false);
    bool include_eql(Env *, ValuePtr);
};

}
