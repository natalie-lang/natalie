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

    ArrayObject(ClassObject *klass)
        : Object { Object::Type::Array, klass } { }

    ArrayObject(size_t initial_capacity)
        : ArrayObject {} {
        m_vector.set_capacity(initial_capacity);
    }

    ArrayObject(std::initializer_list<Value> list)
        : ArrayObject {} {
        m_vector.set_capacity(list.size());
        for (auto v : list) {
            NAT_GC_GUARD_VALUE(v);
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

    ArrayObject(size_t argc, const Value *args)
        : ArrayObject { argc, args, GlobalEnv::the()->Array() } { }

    ArrayObject(size_t argc, const Value *args, ClassObject *klass)
        : Object { Object::Type::Array, klass } {
        m_vector.set_capacity(argc);
        for (size_t i = 0; i < argc; i++) {
            push(args[i]);
        }
    }

    // Array[]
    static Value square_new(Env *env, Args args, ClassObject *klass) {
        return new ArrayObject { args.size(), args.data(), klass };
    }

    static Value size_fn(Env *, Value self, Args, Block *) {
        return Value::integer(self->as_array()->size());
    }

    Value to_ary_method() { return this; }

    size_t size() const { return m_vector.size(); }

    void push(Value val) {
        NAT_GC_GUARD_VALUE(val);
        m_vector.push(val);
    }

    Value first();
    Value last();

    Value pop();
    Value shift();

    Value &at(size_t index) const {
        assert(index < m_vector.size());
        return m_vector[index];
    }

    void set(size_t index, Value value);

    Value &operator[](size_t index) const {
        assert(index < m_vector.size()); // TODO: remove this assertion (audit what's using this operator first though)
        return m_vector[index];
    }

    void concat(ArrayObject &other) {
        for (Value v : other) {
            push(v);
        }
    }

    void truncate(size_t new_size) {
        assert(new_size < size());
        m_vector.set_size(new_size);
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

    const Value *data() const {
        return m_vector.data();
    }

    Value sort_in_place(Env *, Block *);
    Value sort_by_in_place(Env *, Block *);

    Vector<Value>::iterator begin() noexcept { return m_vector.begin(); }
    Vector<Value>::iterator end() noexcept { return m_vector.end(); }

    bool is_empty() { return m_vector.is_empty(); }

    Value initialize(Env *, Value, Value, Block *);

    Value add(Env *, Value);
    Value any(Env *, Args, Block *);
    Value at(Env *, Value);
    Value assoc(Env *, Value);
    Value bsearch(Env *, Block *);
    Value bsearch_index(Env *, Block *);
    Value cmp(Env *, Value);
    Value clear(Env *);
    Value compact(Env *);
    Value compact_in_place(Env *);
    Value concat(Env *, Args);
    Value cycle(Env *, Value, Block *);
    Value delete_at(Env *, Value);
    Value delete_if(Env *, Block *);
    Value delete_item(Env *, Value, Block *);
    Value difference(Env *, Args);
    Value dig(Env *, Args);
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
    Value insert(Env *, Args);
    Value intersection(Env *, Value);
    Value intersection(Env *, Args);
    bool intersects(Env *, Value);
    Value _subjoin(Env *, Value, Value);
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
    Value none(Env *, Args, Block *);
    Value one(Env *, Args, Block *);
    Value pack(Env *, Value, Value);
    Value pop(Env *, Value);
    Value product(Env *, Args, Block *);
    Value push(Env *, Args);
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
    Value sum(Env *, Args, Block *);
    Value union_of(Env *, Value);
    Value union_of(Env *, Args);
    Value uniq(Env *, Block *);
    Value uniq_in_place(Env *, Block *);
    Value unshift(Env *, Args);
    Value to_h(Env *, Block *);
    Value values_at(Env *, Args);
    Value zip(Env *, Args, Block *);

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        for (auto val : m_vector) {
            visitor.visit(val);
        }
    }

    virtual String dbg_inspect() const override;

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
