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

    virtual ~IoBufferObject() override { release_memory(); }

    Value initialize(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value size() const { return Value::integer(static_cast<nat_int_t>(m_size)); }
    Value free(Env *);

    bool is_null() const { return m_base == nullptr; }
    bool is_empty() const { return m_size == 0; }
    bool is_valid() const { return true; }
    bool is_external() const { return m_flags & EXTERNAL; }
    bool is_internal() const { return m_flags & INTERNAL; }
    bool is_mapped() const { return m_flags & MAPPED; }
    bool is_shared() const { return m_flags & SHARED; }
    bool is_locked() const { return m_flags & LOCKED; }
    bool is_private() const { return m_flags & PRIVATE; }
    bool is_readonly() const { return m_flags & READONLY; }

private:
    IoBufferObject(ClassObject *klass)
        : Object { Object::Type::IoBuffer, klass } { }

    void release_memory();

    void *m_base { nullptr };
    size_t m_size { 0 };
    uint32_t m_flags { 0 };
};

}
