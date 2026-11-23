/* Standalone configuration for MRI's Onigmo, compiled with Zig's C compiler. */
#ifndef CORA_ONIGMO_CONFIG_H
#define CORA_ONIGMO_CONFIG_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#define PACKAGE "cora-onigmo"
#define SIZEOF_INT __SIZEOF_INT__
#define SIZEOF_SHORT __SIZEOF_SHORT__
#define SIZEOF_LONG __SIZEOF_LONG__
#define SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
#define SIZEOF_VOIDP __SIZEOF_POINTER__
#define HAVE_ALLOCA_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDLIB_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1
#define TIME_WITH_SYS_TIME 1
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#endif

#define RUBY_SYMBOL_EXPORT_BEGIN
#define RUBY_SYMBOL_EXPORT_END
#define RUBY_ASSERT assert
#define CONSTFUNC(decl) decl __attribute__((const))
#define PUREFUNC(decl) decl __attribute__((pure))
#define RB_GNUC_EXTENSION __extension__
#define RB_GNUC_EXTENSION_BLOCK(statement) __extension__ ({ statement; })
#define UNREACHABLE_RETURN(value) __builtin_unreachable()
#define MEMCPY(destination, source, type, count) \
    memcpy((destination), (source), sizeof(type) * (count))

#endif
