#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class IoBufferObject : public Object {
public:
    enum Flags : uint32_t {
        EXTERNAL = 1,
        INTERNAL = 2,
        MAPPED = 4,
        SHARED = 8,
        LOCKED = 32,
        PRIVATE = 64,
        READONLY = 128,
    };

    static constexpr uint32_t FLAGS_MASK = EXTERNAL | INTERNAL | MAPPED | SHARED | LOCKED | PRIVATE | READONLY;

    enum Endian : uint32_t {
        LITTLE_ENDIAN_FLAG = 4,
        BIG_ENDIAN_FLAG = 8,
    };

    static IoBufferObject *create(ClassObject *klass) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new IoBufferObject(klass);
    }

    static void build_constants(Env *, ClassObject *);
    static long page_size() { return s_page_size; }

    static Value s_for(Env *, ClassObject *, Value, Block *);
    static Value s_string(Env *, ClassObject *, Value, Block *);

    virtual ~IoBufferObject() override { release_memory(); }

    Value initialize(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value size() const { return Value::integer(static_cast<nat_int_t>(m_size)); }
    Value free(Env *);
    Value resize(Env *, Value);
    Value transfer(Env *);
    Value slice(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value to_s(Env *);
    Value get_string(Env *, Optional<Value> = {}, Optional<Value> = {}, Optional<Value> = {});
    Value set_string(Env *, Value, Optional<Value> = {}, Optional<Value> = {}, Optional<Value> = {});
    Value op_not(Env *);
    Value not_bang(Env *);
    Value op_and(Env *, Value);
    Value and_bang(Env *, Value);
    Value op_or(Env *, Value);
    Value or_bang(Env *, Value);
    Value op_xor(Env *, Value);
    Value xor_bang(Env *, Value);

    bool is_null() const { return m_base == nullptr; }
    bool is_empty() const { return m_size == 0; }
    bool is_valid() const;
    bool is_external() const { return m_flags & EXTERNAL; }
    bool is_internal() const { return m_flags & INTERNAL; }
    bool is_mapped() const { return m_flags & MAPPED; }
    bool is_shared() const { return m_flags & SHARED; }
    bool is_locked() const { return m_flags & LOCKED; }
    bool is_private() const { return m_flags & PRIVATE; }
    bool is_readonly() const { return m_flags & READONLY; }

    virtual void visit_children(Visitor &) const override;

private:
    IoBufferObject(ClassObject *klass)
        : Object { Object::Type::IoBuffer, klass } { }

    void release_memory();
    void attach_to_string(StringObject *, uint32_t flags);
    void assert_valid(Env *) const;
    void assert_writable(Env *) const;
    void *allocate_or_raise(Env *, size_t, uint32_t flags);

    void *m_base { nullptr };
    size_t m_size { 0 };
    uint32_t m_flags { 0 };
    Object *m_source { nullptr };

    inline static long s_page_size { 0 };
};

}
