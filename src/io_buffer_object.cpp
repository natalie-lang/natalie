#include "natalie.hpp"
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

namespace Natalie {

static long io_buffer_default_size(long page_size) {
    if (const char *env_override = getenv("RUBY_IO_BUFFER_DEFAULT_SIZE")) {
        const int value = atoi(env_override);
        if (value > 0) return value;
    }
    constexpr long platform_agnostic_default_size = 64 * 1024;
    return platform_agnostic_default_size < page_size ? page_size : platform_agnostic_default_size;
}

void IoBufferObject::build_constants(Env *env, ClassObject *klass) {
    const long page_size = sysconf(_SC_PAGESIZE);
    klass->const_set("PAGE_SIZE"_s, Value::integer(page_size));
    klass->const_set("DEFAULT_SIZE"_s, Value::integer(io_buffer_default_size(page_size)));

    klass->const_set("EXTERNAL"_s, Value::integer(EXTERNAL));
    klass->const_set("INTERNAL"_s, Value::integer(INTERNAL));
    klass->const_set("MAPPED"_s, Value::integer(MAPPED));
    klass->const_set("SHARED"_s, Value::integer(SHARED));
    klass->const_set("LOCKED"_s, Value::integer(LOCKED));
    klass->const_set("PRIVATE"_s, Value::integer(PRIVATE));
    klass->const_set("READONLY"_s, Value::integer(READONLY));

    klass->const_set("LITTLE_ENDIAN"_s, Value::integer(LITTLE_ENDIAN_FLAG));
    klass->const_set("BIG_ENDIAN"_s, Value::integer(BIG_ENDIAN_FLAG));
    klass->const_set("NETWORK_ENDIAN"_s, Value::integer(BIG_ENDIAN_FLAG));

    int probe = 1;
    bool host_is_little_endian = *((char *)&probe) == 1;
    klass->const_set("HOST_ENDIAN"_s, Value::integer(host_is_little_endian ? LITTLE_ENDIAN_FLAG : BIG_ENDIAN_FLAG));
}

void IoBufferObject::release_memory() {
    if (!m_base) return;
    if (m_flags & INTERNAL)
        ::free(m_base);
    else if (m_flags & MAPPED)
        munmap(m_base, m_size);
    m_base = nullptr;
    m_size = 0;
    m_flags = 0;
}

static size_t extract_size(Env *env, Value arg) {
    if (!arg.is_integer())
        env->raise("TypeError", "not an Integer");
    auto integer = arg.integer();
    if (integer.is_negative())
        env->raise("ArgumentError", "Size can't be negative!");
    return static_cast<size_t>(integer.to_nat_int_t());
}

static uint32_t extract_flags(Env *env, Value arg) {
    if (!arg.is_integer())
        env->raise("TypeError", "not an Integer");
    auto integer = arg.integer();
    if (integer.is_negative())
        env->raise("ArgumentError", "Flags can't be negative!");
    return static_cast<uint32_t>(integer.to_nat_int_t()) & IoBufferObject::FLAGS_MASK;
}

Value IoBufferObject::initialize(Env *env, Optional<Value> size_arg, Optional<Value> flags_arg) {
    release_memory();

    size_t size;
    if (size_arg) {
        size = extract_size(env, size_arg.value());
    } else {
        auto default_size = klass()->const_fetch("DEFAULT_SIZE"_s);
        size = static_cast<size_t>(default_size.integer().to_nat_int_t());
    }

    uint32_t flags;
    if (flags_arg) {
        flags = extract_flags(env, flags_arg.value());
    } else {
        const long page_size = sysconf(_SC_PAGESIZE);
        flags = (static_cast<long>(size) >= page_size) ? MAPPED : INTERNAL;
    }

    if (size == 0)
        return this;

    void *base = nullptr;
    if (flags & INTERNAL) {
        base = calloc(size, 1);
    } else if (flags & MAPPED) {
        base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) base = nullptr;
    }

    if (!base) {
        auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
        env->raise(AllocationError, "Could not allocate buffer!");
    }

    m_base = base;
    m_size = size;
    m_flags = flags;
    return this;
}

Value IoBufferObject::free(Env *env) {
    if (m_flags & LOCKED) {
        auto LockedError = klass()->const_fetch("LockedError"_s).as_class();
        env->raise(LockedError, "Buffer is locked!");
    }
    release_memory();
    return this;
}

}
