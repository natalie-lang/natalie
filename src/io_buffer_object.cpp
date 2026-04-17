#include "natalie.hpp"
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
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

static void free_backing(void *base, size_t size, uint32_t flags) {
    if (!base) return;
    if (flags & IoBufferObject::INTERNAL)
        ::free(base);
    else if (flags & IoBufferObject::MAPPED)
        munmap(base, size);
}

static uint32_t choose_flags_for_size(size_t size) {
    return (static_cast<long>(size) >= IoBufferObject::page_size()) ? IoBufferObject::MAPPED : IoBufferObject::INTERNAL;
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
    s_host_is_le = *((char *)&probe) == 1;
    klass->const_set("HOST_ENDIAN"_s, Value::integer(s_host_is_le ? LITTLE_ENDIAN_FLAG : BIG_ENDIAN_FLAG));
}

void IoBufferObject::release_memory() {
    free_backing(m_base, m_size, m_flags);
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

void IoBufferObject::assert_writable(Env *env) const {
    if (!(m_flags & READONLY)) return;
    auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
    env->raise(AccessError, "Buffer is not writable!");
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

void *IoBufferObject::allocate_or_raise(Env *env, size_t size, uint32_t flags) {
    void *base = allocate_buffer(size, flags);
    if (!base) {
        auto AllocationError = klass()->const_fetch("AllocationError"_s).as_class();
        env->raise(AllocationError, "Could not allocate buffer!");
    }
    return base;
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
        flags = choose_flags_for_size(size);
    }

    if (size == 0)
        return this;

    m_base = allocate_or_raise(env, size, flags);
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
        const uint32_t new_flags = choose_flags_for_size(new_size);
        m_base = allocate_or_raise(env, new_size, new_flags);
        m_size = new_size;
        m_flags = new_flags;
        return this;
    }

    if (m_flags & EXTERNAL) {
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
        new_flags = choose_flags_for_size(new_size);
    }

    void *new_base = allocate_or_raise(env, new_size, new_flags);

    const size_t copy_size = std::min(old_size, new_size);
    if (copy_size > 0) memcpy(new_base, old_base, copy_size);

    free_backing(old_base, old_size, old_flags);

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

Value IoBufferObject::locked(Env *env, Block *block) {
    if (!block)
        env->raise("LocalJumpError", "no block given");
    if (m_flags & LOCKED) {
        auto LockedError = klass()->const_fetch("LockedError"_s).as_class();
        env->raise(LockedError, "Buffer already locked!");
    }

    m_flags |= LOCKED;
    Value result;
    try {
        result = block->run(env, { this }, nullptr);
    } catch (ExceptionObject *exception) {
        m_flags &= ~LOCKED;
        throw exception;
    }
    m_flags &= ~LOCKED;
    return result;
}

Value IoBufferObject::slice(Env *env, Optional<Value> offset_arg, Optional<Value> length_arg) {
    const size_t offset = offset_arg ? extract_offset(env, offset_arg.value()) : 0;
    const size_t length = length_arg ? extract_length(env, length_arg.value()) : (m_size > offset ? m_size - offset : 0);

    if (offset + length > m_size)
        env->raise("ArgumentError", "Specified offset+length exceeds source size!");

    auto slice = IoBufferObject::create(klass());
    slice->m_base = m_base ? static_cast<char *>(m_base) + offset : nullptr;
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

Value IoBufferObject::s_map(Env *env, ClassObject *klass, Value io_arg, Optional<Value> size_arg, Optional<Value> offset_arg, Optional<Value> flags_arg) {
    if (!io_arg.is_io())
        env->raise("TypeError", "wrong argument type {} (expected IO)", io_arg.klass()->inspect_module());
    auto io = io_arg.as_io();
    const int fd = io->fileno();

    struct stat st;
    if (fstat(fd, &st) < 0) env->raise_errno();
    const off_t file_size = st.st_size;
    if (file_size <= 0)
        env->raise("ArgumentError", "Invalid negative or zero file size!");

    const bool size_nil = !size_arg || size_arg.value().is_nil();
    size_t size;
    if (!size_nil) {
        size = extract_size(env, size_arg.value());
        if (size == 0)
            env->raise("ArgumentError", "Size can't be zero!");
        if (size > static_cast<size_t>(file_size))
            env->raise("ArgumentError", "Size can't be larger than file size!");
    } else {
        size = static_cast<size_t>(file_size);
    }

    off_t offset = 0;
    if (offset_arg) {
        offset = static_cast<off_t>(offset_arg.value().to_int(env).to_nat_int_t());
        if (offset < 0)
            env->raise("ArgumentError", "Offset can't be negative!");
        if (offset >= file_size)
            env->raise("ArgumentError", "Offset too large!");
        if (size_nil) {
            size = static_cast<size_t>(file_size - offset);
        } else if (static_cast<size_t>(file_size - offset) < size) {
            env->raise("ArgumentError", "Offset too large!");
        }
    }

    uint32_t requested_flags = 0;
    if (flags_arg) requested_flags = extract_flags(env, flags_arg.value());

    int prot = PROT_READ;
    int mmap_flags = 0;
    uint32_t buffer_flags = MAPPED;

    if (requested_flags & READONLY) {
        buffer_flags |= READONLY;
    } else {
        prot |= PROT_WRITE;
    }

    if (requested_flags & PRIVATE) {
        buffer_flags |= PRIVATE;
        mmap_flags |= MAP_PRIVATE;
    } else {
        buffer_flags |= EXTERNAL | SHARED;
        mmap_flags |= MAP_SHARED;
    }

    void *base = mmap(nullptr, size, prot, mmap_flags, fd, offset);
    if (base == MAP_FAILED) env->raise_errno();

    auto buffer = IoBufferObject::create(klass);
    buffer->m_base = base;
    buffer->m_size = size;
    buffer->m_flags = buffer_flags;
    return buffer;
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
        const uint32_t new_flags = choose_flags_for_size(m_size);
        void *base = allocate_or_raise(env, m_size, new_flags);
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
    assert_writable(env);
    assert_valid(env);

    auto *base = static_cast<unsigned char *>(m_base);
    for (size_t i = 0; i < m_size; i++)
        base[i] = ~base[i];
    return this;
}

static IoBufferObject *assert_buffer_arg(Env *env, Value arg) {
    if (!arg.is_pointer() || arg->type() != Object::Type::IoBuffer) {
        if (arg.is_nil())
            env->raise("TypeError", "wrong argument type nil (expected IO::Buffer)");
        env->raise("TypeError", "wrong argument type {} (expected IO::Buffer)", arg.klass()->inspect_module());
    }
    return static_cast<IoBufferObject *>(arg.object());
}

Value IoBufferObject::op_and(Env *env, Value mask_arg) {
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    auto new_buffer = IoBufferObject::create(klass());
    if (m_size > 0 && mask->m_size > 0) {
        const uint32_t new_flags = choose_flags_for_size(m_size);
        void *base = allocate_or_raise(env, m_size, new_flags);
        const auto *src = static_cast<const unsigned char *>(m_base);
        const auto *m = static_cast<const unsigned char *>(mask->m_base);
        auto *dst = static_cast<unsigned char *>(base);
        for (size_t i = 0; i < m_size; i++)
            dst[i] = src[i] & m[i % mask->m_size];
        new_buffer->m_base = base;
        new_buffer->m_size = m_size;
        new_buffer->m_flags = new_flags;
    }
    return new_buffer;
}

Value IoBufferObject::and_bang(Env *env, Value mask_arg) {
    assert_writable(env);
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    if (mask->m_size == 0) return this;
    auto *base = static_cast<unsigned char *>(m_base);
    const auto *m = static_cast<const unsigned char *>(mask->m_base);
    for (size_t i = 0; i < m_size; i++)
        base[i] = base[i] & m[i % mask->m_size];
    return this;
}

Value IoBufferObject::op_or(Env *env, Value mask_arg) {
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    auto new_buffer = IoBufferObject::create(klass());
    if (m_size > 0 && mask->m_size > 0) {
        const uint32_t new_flags = choose_flags_for_size(m_size);
        void *base = allocate_or_raise(env, m_size, new_flags);
        const auto *src = static_cast<const unsigned char *>(m_base);
        const auto *m = static_cast<const unsigned char *>(mask->m_base);
        auto *dst = static_cast<unsigned char *>(base);
        for (size_t i = 0; i < m_size; i++)
            dst[i] = src[i] | m[i % mask->m_size];
        new_buffer->m_base = base;
        new_buffer->m_size = m_size;
        new_buffer->m_flags = new_flags;
    }
    return new_buffer;
}

Value IoBufferObject::or_bang(Env *env, Value mask_arg) {
    assert_writable(env);
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    if (mask->m_size == 0) return this;
    auto *base = static_cast<unsigned char *>(m_base);
    const auto *m = static_cast<const unsigned char *>(mask->m_base);
    for (size_t i = 0; i < m_size; i++)
        base[i] = base[i] | m[i % mask->m_size];
    return this;
}

Value IoBufferObject::op_xor(Env *env, Value mask_arg) {
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    auto new_buffer = IoBufferObject::create(klass());
    if (m_size > 0 && mask->m_size > 0) {
        const uint32_t new_flags = choose_flags_for_size(m_size);
        void *base = allocate_or_raise(env, m_size, new_flags);
        const auto *src = static_cast<const unsigned char *>(m_base);
        const auto *m = static_cast<const unsigned char *>(mask->m_base);
        auto *dst = static_cast<unsigned char *>(base);
        for (size_t i = 0; i < m_size; i++)
            dst[i] = src[i] ^ m[i % mask->m_size];
        new_buffer->m_base = base;
        new_buffer->m_size = m_size;
        new_buffer->m_flags = new_flags;
    }
    return new_buffer;
}

Value IoBufferObject::xor_bang(Env *env, Value mask_arg) {
    assert_writable(env);
    assert_valid(env);
    auto mask = assert_buffer_arg(env, mask_arg);
    mask->assert_valid(env);

    if (mask->m_size == 0) return this;
    auto *base = static_cast<unsigned char *>(m_base);
    const auto *m = static_cast<const unsigned char *>(mask->m_base);
    for (size_t i = 0; i < m_size; i++)
        base[i] = base[i] ^ m[i % mask->m_size];
    return this;
}

namespace {

struct DataType {
    size_t width;
    bool is_signed;
    bool is_float;
    bool is_le;

    Value read(const unsigned char *src, bool host_le) const {
        const bool swap = width > 1 && is_le != host_le;

        switch (width) {
        case 1:
            return is_signed
                ? Value::integer(static_cast<nat_int_t>(static_cast<int8_t>(src[0])))
                : Value::integer(static_cast<nat_int_t>(src[0]));
        case 2: {
            uint16_t v;
            memcpy(&v, src, 2);
            if (swap) v = __builtin_bswap16(v);
            if (is_signed) return Value::integer(static_cast<nat_int_t>(static_cast<int16_t>(v)));
            return Value::integer(static_cast<nat_int_t>(v));
        }
        case 4: {
            uint32_t v;
            memcpy(&v, src, 4);
            if (swap) v = __builtin_bswap32(v);
            if (is_float) {
                float f;
                memcpy(&f, &v, 4);
                return FloatObject::create(static_cast<double>(f));
            }
            if (is_signed) return Value::integer(static_cast<nat_int_t>(static_cast<int32_t>(v)));
            return Value::integer(static_cast<nat_int_t>(v));
        }
        case 8: {
            uint64_t v;
            memcpy(&v, src, 8);
            if (swap) v = __builtin_bswap64(v);
            if (is_float) {
                double f;
                memcpy(&f, &v, 8);
                return FloatObject::create(f);
            }
            if (is_signed) return Value::integer(static_cast<nat_int_t>(static_cast<int64_t>(v)));
            return Value::integer(static_cast<nat_int_t>(v));
        }
        default:
            NAT_UNREACHABLE();
        }
    }

    void write(unsigned char *dst, Env *env, Value val, bool host_le) const {
        const bool swap = width > 1 && is_le != host_le;

        switch (width) {
        case 1: {
            auto v = static_cast<uint8_t>(IntegerMethods::convert_to_nat_int_t(env, val));
            dst[0] = v;
            break;
        }
        case 2: {
            auto v = static_cast<uint16_t>(IntegerMethods::convert_to_nat_int_t(env, val));
            if (swap) v = __builtin_bswap16(v);
            memcpy(dst, &v, 2);
            break;
        }
        case 4: {
            if (is_float) {
                auto f = static_cast<float>(val.to_f(env)->to_double());
                uint32_t v;
                memcpy(&v, &f, 4);
                if (swap) v = __builtin_bswap32(v);
                memcpy(dst, &v, 4);
            } else {
                auto v = static_cast<uint32_t>(IntegerMethods::convert_to_nat_int_t(env, val));
                if (swap) v = __builtin_bswap32(v);
                memcpy(dst, &v, 4);
            }
            break;
        }
        case 8: {
            if (is_float) {
                auto f = val.to_f(env)->to_double();
                uint64_t v;
                memcpy(&v, &f, 8);
                if (swap) v = __builtin_bswap64(v);
                memcpy(dst, &v, 8);
            } else {
                auto v = static_cast<uint64_t>(IntegerMethods::convert_to_nat_int_t(env, val));
                if (swap) v = __builtin_bswap64(v);
                memcpy(dst, &v, 8);
            }
            break;
        }
        default:
            NAT_UNREACHABLE();
        }
    }
};

static constexpr struct {
    const char *name;
    DataType dt;
} data_type_table[] = {
    { "U8", { 1, false, false, false } },
    { "S8", { 1, true, false, false } },
    { "u16", { 2, false, false, true } },
    { "U16", { 2, false, false, false } },
    { "s16", { 2, true, false, true } },
    { "S16", { 2, true, false, false } },
    { "u32", { 4, false, false, true } },
    { "U32", { 4, false, false, false } },
    { "s32", { 4, true, false, true } },
    { "S32", { 4, true, false, false } },
    { "u64", { 8, false, false, true } },
    { "U64", { 8, false, false, false } },
    { "s64", { 8, true, false, true } },
    { "S64", { 8, true, false, false } },
    { "f32", { 4, false, true, true } },
    { "F32", { 4, false, true, false } },
    { "f64", { 8, false, true, true } },
    { "F64", { 8, false, true, false } },
};

const DataType *find_data_type(Env *env, const String &name) {
    for (const auto &entry : data_type_table) {
        if (name == entry.name) return &entry.dt;
    }
    env->raise("ArgumentError", "Invalid type name!");
}

} // anonymous namespace

Value IoBufferObject::get_value(Env *env, Value type_arg, Value offset_arg) {
    assert_valid(env);
    auto sym = type_arg.to_symbol(env, Value::Conversion::Strict);
    const auto *dt = find_data_type(env, sym->string());
    const size_t offset = extract_offset(env, offset_arg);

    if (offset + dt->width > m_size) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Specified offset+type exceeds source buffer size!");
    }

    return dt->read(static_cast<const unsigned char *>(m_base) + offset, s_host_is_le);
}

Value IoBufferObject::set_value(Env *env, Value type_arg, Value offset_arg, Value value_arg) {
    assert_writable(env);
    assert_valid(env);
    auto sym = type_arg.to_symbol(env, Value::Conversion::Strict);
    const auto *dt = find_data_type(env, sym->string());
    const size_t offset = extract_offset(env, offset_arg);

    if (offset + dt->width > m_size) {
        auto AccessError = klass()->const_fetch("AccessError"_s).as_class();
        env->raise(AccessError, "Specified offset+type exceeds target buffer size!");
    }

    dt->write(static_cast<unsigned char *>(m_base) + offset, env, value_arg, s_host_is_le);
    return Value::integer(static_cast<nat_int_t>(offset + dt->width));
}

Value IoBufferObject::each(Env *env, Optional<Value> type_arg, Optional<Value> offset_arg, Optional<Value> count_arg, Block *block) {
    auto type = type_arg ? type_arg.value().to_symbol(env, Value::Conversion::Strict) : "U8"_s;

    if (!block)
        return send(env, "enum_for"_s, { "each"_s, type });

    assert_valid(env);

    const auto *dt = find_data_type(env, type->string());
    size_t offset = offset_arg ? extract_offset(env, offset_arg.value()) : 0;
    size_t count;
    if (count_arg) {
        count = extract_length(env, count_arg.value());
    } else {
        count = m_size > offset ? (m_size - offset) / dt->width : 0;
    }

    for (size_t i = 0; i < count; i++) {
        size_t current_offset = offset + i * dt->width;
        Value value = get_value(env, type, Value::integer(static_cast<nat_int_t>(current_offset)));
        block->run(env, { Value::integer(static_cast<nat_int_t>(current_offset)), value }, nullptr);
    }
    return this;
}

Value IoBufferObject::set_string(Env *env, Value source_arg, Optional<Value> offset_arg, Optional<Value> length_arg, Optional<Value> source_offset_arg) {
    assert_writable(env);
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
