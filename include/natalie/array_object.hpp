#pragma once

#include <initializer_list>
#include <utility>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "tm/recursion_guard.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

class ArrayObject : public Object {
public:
    ArrayObject()
        : Object { Object::Type::Array, GlobalEnv::the()->Array() } { }

    ArrayObject(size_t initial_capacity)
        : ArrayObject {} {
        m_vector.set_capacity(initial_capacity);
    }

    ArrayObject(std::initializer_list<Value> list)
        : ArrayObject {} {
        m_vector.set_capacity(list.size());
        for (auto &v : list) {
            m_vector.push(v);
        }
    }

    ArrayObject(const ArrayObject &other)
        : Object { other }
        , m_vector { other.m_vector } { }

    ArrayObject &operator=(ArrayObject &&other) {
        Object::operator=(std::move(other));
        m_vector = std::move(other.m_vector);
        return *this;
    }

    ArrayObject(size_t argc, Value *args)
        : ArrayObject { argc, args, GlobalEnv::the()->Array() } { }

    ArrayObject(size_t argc, Value *args, ClassObject *klass)
        : Object { Object::Type::Array, klass } {
        m_vector.set_capacity(argc);
        for (size_t i = 0; i < argc; i++) {
            push(args[i]);
        }
    }

    // Array[]
    static Value square_new(Env *env, size_t argc, Value *args, ClassObject *klass) {
        return new ArrayObject { argc, args, klass };
    }

    Value to_ary_method() { return this; }

    size_t size() const { return m_vector.size(); }

    void push(Object &val) {
        m_vector.push(&val);
    }

    void push(Value val) {
        m_vector.push(val);
    }

    Value first();
    Value last();

    Value pop(Env *, Value);

    Value &at(size_t index) const {
        assert(index < m_vector.size());
        return m_vector[index];
    }

    Value &operator[](size_t index) const {
        assert(index < m_vector.size()); // TODO: remove this assertion (audit what's using this operator first though)
        return m_vector[index];
    }

    void concat(ArrayObject &other) {
        for (Value v : other) {
            push(v);
        }
    }

    void push_splat(Env *, Value);

    void expand_with_nil(Env *, size_t);

    void overwrite(ArrayObject &other) {
        m_vector.set_size(0);
        for (Value v : other) {
            push(v);
        }
    }

    Value *data() {
        return m_vector.data();
    }

    Value sort_in_place(Env *, Block *);
    Value sort_by_in_place(Env *, Block *);

    Vector<Value>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<Value>::iterator end() noexcept { return m_vector.end(); }

    bool is_empty() { return m_vector.is_empty(); }

    Value initialize(Env *, Value, Value, Block *);

    Value add(Env *, Value);
    Value any(Env *, size_t, Value *, Block *);
    Value at(Env *, Value);
    Value assoc(Env *, Value);
    Value bsearch(Env *, Block *);
    Value bsearch_index(Env *, Block *);
    Value cmp(Env *, Value);
    Value clear(Env *);
    Value compact(Env *);
    Value compact_in_place(Env *);
    Value concat(Env *, size_t, Value *);
    Value cycle(Env *, Value, Block *);
    Value delete_at(Env *, Value);
    Value delete_if(Env *, Block *);
    Value delete_item(Env *, Value, Block *);
    Value difference(Env *, size_t, Value *);
    Value dig(Env *, size_t, Value *);
    Value drop(Env *, Value);
    Value drop_while(Env *, Block *);
    Value each(Env *, Block *);
    Value each_index(Env *, Block *);
    bool eq(Env *, Value);
    bool eql(Env *, Value);
    Value fetch(Env *, Value, Value, Block *);
    Value fill(Env *, Value, Value, Value, Block *);
    Value first(Env *, Value);
    Value flatten(Env *, Value);
    Value flatten_in_place(Env *, Value);
    Value hash(Env *);
    bool include(Env *, Value);
    Value index(Env *, Value, Block *);
    Value initialize_copy(Env *, Value);
    Value inspect(Env *);
    Value insert(Env *, size_t, Value *);
    Value intersection(Env *, Value);
    Value intersection(Env *, size_t, Value *);
    Value join(Env *, Value);
    Value keep_if(Env *, Block *);
    Value last(Env *, Value);
    Value ltlt(Env *, Value);
    Value map(Env *, Block *);
    Value map_in_place(Env *, Block *);
    Value max(Env *, Value, Block *);
    Value min(Env *, Value, Block *);
    Value minmax(Env *, Block *);
    Value multiply(Env *, Value);
    Value none(Env *, size_t, Value *, Block *);
    Value one(Env *, size_t, Value *, Block *);
    Value pack(Env *, Value);
    Value product(Env *, size_t, Value *, Block *);
    Value push(Env *, size_t, Value *);
    Value rassoc(Env *, Value);
    Value ref(Env *, Value, Value = nullptr);
    Value refeq(Env *, Value, Value, Value);
    Value reject(Env *, Block *);
    Value reject_in_place(Env *, Block *);
    Value reverse(Env *);
    Value reverse_each(Env *, Block *);
    Value reverse_in_place(Env *);
    Value rindex(Env *, Value, Block *);
    Value rotate(Env *, Value);
    Value rotate_in_place(Env *, Value);
    Value select(Env *, Block *);
    Value select_in_place(Env *, Block *);
    bool select_in_place(std::function<bool(Value &)>);
    Value shift(Env *, Value);
    Value slice_in_place(Env *, Value, Value);
    Value sort(Env *, Block *);
    Value sub(Env *, Value);
    static Value try_convert(Env *, Value);
    Value sum(Env *, size_t, Value *, Block *);
    Value union_of(Env *, Value);
    Value union_of(Env *, size_t, Value *);
    Value uniq(Env *, Block *);
    Value uniq_in_place(Env *, Block *);
    Value unshift(Env *, size_t, Value *);
    Value to_h(Env *, Block *);
    Value values_at(Env *, size_t, Value *);
    Value zip(Env *, size_t, Value *, Block *);

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        for (auto val : m_vector) {
            visitor.visit(val);
        }
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        size_t size = m_vector.size();
        snprintf(buf, len, "<ArrayObject %p size=%zu>", this, size);
    }

private:
    ArrayObject(Vector<Value> &&vector)
        : Object { Object::Type::Array, GlobalEnv::the()->Array() }
        , m_vector { std::move(vector) } { }

    Vector<Value> m_vector {};

    nat_int_t _resolve_index(nat_int_t) const;
    bool _flatten_in_place(Env *, nat_int_t depth, Hashmap<ArrayObject *> visited_arrays = Hashmap<ArrayObject *> {});
    Value _slice_in_place(nat_int_t start, nat_int_t end, bool exclude_end);
    Value find_index(Env *, Value, Block *, bool = false);
    bool include_eql(Env *, Value);
};

}
