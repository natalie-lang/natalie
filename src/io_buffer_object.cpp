#include "natalie.hpp"
#include <algorithm>
#include <stdlib.h>
#include <string.h>
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
    s_page_size = sysconf(_SC_PAGESIZE);
    klass->const_set("PAGE_SIZE"_s, Value::integer(s_page_size));
    klass->const_set("DEFAULT_SIZE"_s, Value::integer(io_buffer_default_size(s_page_size)));

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
    if (m_base) {
        if (m_flags & INTERNAL)
            ::free(m_base);
        else if (m_flags & MAPPED)
            munmap(m_base, m_size);
    }
    m_base = nullptr;
    m_size = 0;
    m_flags = 0;
    m_source = nullptr;
}

void IoBufferObject::attach_to_string(StringObject *string, uint32_t flags) {
    m_base = const_cast<char *>(string->c_str());
    m_size = string->bytesize();
    m_flags = flags;
    m_source = string;
}

void IoBufferObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    visitor.visit(m_source);
}

bool IoBufferObject::is_valid() const {
    if (!m_source) return true;

    const char *our_base = static_cast<const char *>(m_base);
    const char *source_base;
    size_t source_size;

    if (m_source->type() == Object::Type::String) {
        auto s = static_cast<const StringObject *>(m_source);
        source_base = s->c_str();
        source_size = s->bytesize();
    } else {
        auto b = static_cast<const IoBufferObject *>(m_source);
        if (!b->m_base) return false;
        source_base = static_cast<const char *>(b->m_base);
        source_size = b->m_size;
    }

    return our_base >= source_base && our_base + m_size <= source_base + source_size;
}

void IoBufferObject::assert_valid(Env *env) const {
    if (is_valid()) return;
    auto InvalidatedError = klass()->const_fetch("InvalidatedError"_s).as_class();
    env->raise(InvalidatedError, "Buffer has been invalidated!");
}

static size_t extract_non_negative_integer(Env *env, Value arg, const char *negative_message) {
    if (!arg.is_integer())
        env->raise("TypeError", "not an Integer");
    auto integer = arg.integer();
    if (integer.is_negative())
        env->raise("ArgumentError", negative_message);
    auto val = integer.to_nat_int_t_or_none();
    if (!val)
        env->raise("RangeError", "bignum too big to convert into `long'");
    return static_cast<size_t>(val.value());
}

static size_t extract_size(Env *env, Value arg) {
    return extract_non_negative_integer(env, arg, "Size can't be negative!");
}

static uint32_t extract_flags(Env *env, Value arg) {
    auto value = extract_non_negative_integer(env, arg, "Flags can't be negative!");
    return static_cast<uint32_t>(value) & IoBufferObject::FLAGS_MASK;
}

static size_t extract_offset(Env *env, Value arg) {
    return extract_non_negative_integer(env, arg, "Offset can't be negative!");
}

static size_t extract_length(Env *env, Value arg) {
    return extract_non_negative_integer(env, arg, "Length can't be negative!");
}

static void *allocate_buffer(size_t size, uint32_t flags) {
    if (flags & IoBufferObject::INTERNAL) return calloc(size, 1);
    if (flags & IoBufferObject::MAPPED) {
        void *base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) return nullptr;
        return base;
    }
    return nullptr;
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
        flags = (static_cast<long>(size) >= s_page_size) ? MAPPED : INTERNAL;
    }

    if (size == 0)
        return this;

    void *base = allocate_buffer(size, flags);
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

Value IoBufferObject::resize(Env *env, Value new_size_arg) {
    if (m_flags & LOCKED) {
        auto LockedError = klass()->const_fetch("LockedError"_s).as_class();
        env->raise(LockedError, "Cannot resize locked buffer!");
    }

    const size_t new_size = extract_size(env, new_size_arg);

    if (new_size == 0) {
        release_memory();
        return this;
    }

    if (!m_base) {
        uint32_t new_flags = (static_cast<long>(new_size) >= s_page_size) ? MAPPED : INTERNAL;
        void *base = allocate_buffer(new_size, new_flags);
        if (!base) {
            auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
            env->raise(AllocationError, "Could not allocate buffer!");
        }
        m_base = base;
        m_size = new_size;
        m_flags = new_flags;
        return this;
    }

    if (!(m_flags & (INTERNAL | MAPPED))) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Cannot resize external buffer!");
    }

    const size_t old_size = m_size;
    void *old_base = m_base;
    const uint32_t old_flags = m_flags;

    uint32_t new_flags;
    if (old_flags & INTERNAL) {
        new_flags = INTERNAL;
    } else {
        new_flags = (static_cast<long>(new_size) >= s_page_size) ? MAPPED : INTERNAL;
    }

    void *new_base = allocate_buffer(new_size, new_flags);
    if (!new_base) {
        auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
        env->raise(AllocationError, "Could not allocate buffer!");
    }

    const size_t copy_size = std::min(old_size, new_size);
    if (copy_size > 0) memcpy(new_base, old_base, copy_size);

    if (old_flags & INTERNAL)
        ::free(old_base);
    else if (old_flags & MAPPED)
        munmap(old_base, old_size);

    m_base = new_base;
    m_size = new_size;
    m_flags = new_flags;
    return this;
}

Value IoBufferObject::transfer(Env *env) {
    if (m_flags & LOCKED) {
        auto LockedError = klass()->const_fetch("LockedError"_s).as_class();
        env->raise(LockedError, "Cannot transfer ownership of locked buffer!");
    }

    auto new_buffer = IoBufferObject::create(klass());
    new_buffer->m_base = m_base;
    new_buffer->m_size = m_size;
    new_buffer->m_flags = m_flags;
    new_buffer->m_source = m_source;

    m_base = nullptr;
    m_size = 0;
    m_flags = 0;
    m_source = nullptr;

    return new_buffer;
}

Value IoBufferObject::slice(Env *env, Optional<Value> offset_arg, Optional<Value> length_arg) {
    if (!m_base) {
        auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
        env->raise(AllocationError, "The buffer is not allocated!");
    }

    const size_t offset = offset_arg ? extract_offset(env, offset_arg.value()) : 0;
    const size_t length = length_arg ? extract_length(env, length_arg.value()) : (m_size > offset ? m_size - offset : 0);

    if (offset + length > m_size)
        env->raise("ArgumentError", "Specified offset+length exceeds source size!");

    auto slice = IoBufferObject::create(klass());
    slice->m_base = static_cast<char *>(m_base) + offset;
    slice->m_size = length;
    slice->m_flags = (m_flags | EXTERNAL) & ~(INTERNAL | MAPPED);
    slice->m_source = m_source ? m_source : static_cast<Object *>(this);
    return slice;
}

Value IoBufferObject::s_for(Env *env, ClassObject *klass, Value string_arg, Block *block) {
    string_arg.assert_type(env, Object::Type::String, "String");
    auto string = string_arg.as_string();

    auto buffer = IoBufferObject::create(klass);

    if (block) {
        uint32_t flags = string->is_frozen() ? (EXTERNAL | READONLY) : EXTERNAL;
        buffer->attach_to_string(string, flags);
        Value result;
        try {
            result = block->run(env, { buffer }, nullptr);
        } catch (ExceptionObject *exception) {
            buffer->release_memory();
            throw exception;
        }
        buffer->release_memory();
        return result;
    }

    auto copy = StringObject::create(*string);
    copy->freeze();
    buffer->attach_to_string(copy, EXTERNAL | READONLY);
    return buffer;
}

Value IoBufferObject::s_string(Env *env, ClassObject *klass, Value length_arg, Block *block) {
    if (!block)
        env->raise("LocalJumpError", "no block given");
    auto size = extract_non_negative_integer(env, length_arg, "negative string size (or size too big)");

    auto string = StringObject::create(String(size, '\0'), Encoding::ASCII_8BIT);
    auto buffer = IoBufferObject::create(klass);
    buffer->attach_to_string(string, EXTERNAL);

    try {
        block->run(env, { buffer }, nullptr);
    } catch (ExceptionObject *exception) {
        buffer->release_memory();
        throw exception;
    }
    buffer->release_memory();
    return string;
}

Value IoBufferObject::to_s(Env *env) {
    if (!m_base)
        return StringObject::create("", Encoding::ASCII_8BIT);
    return StringObject::create(static_cast<const char *>(m_base), m_size, Encoding::ASCII_8BIT);
}

Value IoBufferObject::get_string(Env *env, Optional<Value> offset_arg, Optional<Value> length_arg, Optional<Value> encoding_arg) {
    assert_valid(env);

    size_t offset = offset_arg ? extract_offset(env, offset_arg.value()) : 0;
    size_t length = length_arg ? extract_length(env, length_arg.value()) : (m_size > offset ? m_size - offset : 0);

    if (offset + length > m_size) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Specified offset+length exceeds source size!");
    }

    EncodingObject *encoding = EncodingObject::get(Encoding::ASCII_8BIT);
    if (encoding_arg && !encoding_arg.value().is_nil()) {
        encoding = EncodingObject::find_encoding(env, encoding_arg.value());
    }

    if (!m_base || length == 0)
        return StringObject::create("", encoding);

    return StringObject::create(static_cast<const char *>(m_base) + offset, length, encoding);
}

Value IoBufferObject::op_not(Env *env) {
    assert_valid(env);

    auto new_buffer = IoBufferObject::create(klass());
    if (m_size > 0) {
        uint32_t new_flags = (static_cast<long>(m_size) >= s_page_size) ? MAPPED : INTERNAL;
        void *base = allocate_buffer(m_size, new_flags);
        if (!base) {
            auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
            env->raise(AllocationError, "Could not allocate buffer!");
        }
        const auto *src = static_cast<const unsigned char *>(m_base);
        auto *dst = static_cast<unsigned char *>(base);
        for (size_t i = 0; i < m_size; i++)
            dst[i] = ~src[i];
        new_buffer->m_base = base;
        new_buffer->m_size = m_size;
        new_buffer->m_flags = new_flags;
    }
    return new_buffer;
}

Value IoBufferObject::not_bang(Env *env) {
    if (m_flags & READONLY) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Buffer is not writable!");
    }
    assert_valid(env);

    auto *base = static_cast<unsigned char *>(m_base);
    for (size_t i = 0; i < m_size; i++)
        base[i] = ~base[i];
    return this;
}

Value IoBufferObject::set_string(Env *env, Value source_arg, Optional<Value> offset_arg, Optional<Value> length_arg, Optional<Value> source_offset_arg) {
    if (m_flags & READONLY) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Buffer is not writable!");
    }
    assert_valid(env);

    source_arg.assert_type(env, Object::Type::String, "String");
    auto source = source_arg.as_string();

    size_t offset = offset_arg ? extract_offset(env, offset_arg.value()) : 0;
    size_t source_offset = source_offset_arg ? extract_offset(env, source_offset_arg.value()) : 0;

    size_t max_length = source->bytesize() > source_offset ? source->bytesize() - source_offset : 0;
    size_t length = length_arg ? extract_length(env, length_arg.value()) : max_length;
    if (length > max_length) length = max_length;

    if (offset + length > m_size) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Specified offset+length exceeds target size!");
    }

    if (length > 0)
        memcpy(static_cast<char *>(m_base) + offset, source->c_str() + source_offset, length);

    return Value::integer(static_cast<nat_int_t>(length));
}

}
