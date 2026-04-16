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

    enum Endian : uint32_t {
        LITTLE_ENDIAN_FLAG = 4,
        BIG_ENDIAN_FLAG = 8,
    };

    static IoBufferObject *create(ClassObject *klass) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new IoBufferObject(klass);
    }

    static void build_constants(Env *, ClassObject *);

private:
    IoBufferObject(ClassObject *klass)
        : Object { Object::Type::IoBuffer, klass } { }
};

}
