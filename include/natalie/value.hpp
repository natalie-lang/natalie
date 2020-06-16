#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

struct Value : public gc {
    enum class Type {
        Nil,
        Array,
        Class,
        Encoding,
        Exception,
        False,
        Hash,
        Integer,
        Io,
        MatchData,
        Module,
        Object,
        Proc,
        Range,
        Regexp,
        String,
        Symbol,
        True,
        VoidP,
    };

    enum class Conversion {
        Strict,
        NullAllowed,
    };

    Value(Env *env)
        : klass { NAT_OBJECT } {
        assert(klass);
    }

    Value(ClassValue *klass)
        : klass { klass } {
        assert(klass);
    }

    Value(Type type, ClassValue *klass)
        : type { type }
        , klass { klass } {
        assert(klass);
    }

    Value(const Value &) = delete;
    Value &operator=(const Value &) = delete;

    Type type { Type::Object };
    ClassValue *klass { nullptr };

    ClassValue *m_singleton_class { nullptr };

    ModuleValue *owner { nullptr };
    int flags { 0 };

    Env env;

    struct hashmap ivars EMPTY_HASHMAP;

    Value *initialize(Env *, ssize_t, Value **, Block *);

    bool is_nil() const { return type == Type::Nil; }
    bool is_true() const { return type == Type::True; }
    bool is_false() const { return type == Type::False; }
    bool is_array() const { return type == Type::Array; }
    bool is_module() const { return type == Type::Module || type == Type::Class; }
    bool is_class() const { return type == Type::Class; }
    bool is_encoding() const { return type == Type::Encoding; }
    bool is_exception() const { return type == Type::Exception; }
    bool is_hash() const { return type == Type::Hash; }
    bool is_integer() const { return type == Type::Integer; }
    bool is_io() const { return type == Type::Io; }
    bool is_match_data() const { return type == Type::MatchData; }
    bool is_proc() const { return type == Type::Proc; }
    bool is_range() const { return type == Type::Range; }
    bool is_regexp() const { return type == Type::Regexp; }
    bool is_symbol() const { return type == Type::Symbol; }
    bool is_string() const { return type == Type::String; }
    bool is_void_p() const { return type == Type::VoidP; }

    bool is_truthy() const { return !this->is_false() && !this->is_nil(); }

    NilValue *as_nil();
    TrueValue *as_true();
    FalseValue *as_false();
    ArrayValue *as_array();
    ModuleValue *as_module();
    ClassValue *as_class();
    EncodingValue *as_encoding();
    ExceptionValue *as_exception();
    HashValue *as_hash();
    IntegerValue *as_integer();
    IoValue *as_io();
    MatchDataValue *as_match_data();
    ProcValue *as_proc();
    RangeValue *as_range();
    RegexpValue *as_regexp();
    StringValue *as_string();
    SymbolValue *as_symbol();
    VoidPValue *as_void_p();

    SymbolValue *to_symbol(Env *, Conversion);

    const char *identifier_str(Env *, Conversion);

    ClassValue *singleton_class() { return m_singleton_class; }
    ClassValue *singleton_class(Env *);

    void set_singleton_class(ClassValue *c) { m_singleton_class = c; }

    virtual Value *const_get(Env *, const char *, bool);
    virtual Value *const_get_or_null(Env *, const char *, bool, bool);
    virtual Value *const_set(Env *, const char *, Value *);

    Value *ivar_get(Env *, const char *);
    Value *ivar_set(Env *, const char *, Value *);

    Value *cvar_get(Env *, const char *);
    virtual Value *cvar_get_or_null(Env *, const char *);
    virtual Value *cvar_set(Env *, const char *, Value *);

    virtual void define_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block));
    virtual void define_method_with_block(Env *, const char *, Block *);
    virtual void undefine_method(Env *, const char *);

    void define_singleton_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block));
    void define_singleton_method_with_block(Env *, const char *, Block *);
    void undefine_singleton_method(Env *, const char *);

    virtual void alias(Env *, const char *, const char *);

    int64_t object_id() { return (int64_t)this; }

    void pointer_id(char *buf) {
        snprintf(buf, NAT_OBJECT_POINTER_BUF_LENGTH, "%p", this);
    }

    Value *send(Env *, const char *, ssize_t = 0, Value ** = nullptr, Block * = nullptr);

    Value *dup(Env *);

    bool is_a(Env *, Value *);
    bool respond_to(Env *, const char *);

    const char *defined(Env *, const char *);
    Value *defined_obj(Env *, const char *);

    virtual ProcValue *to_proc(Env *);

private:
    void init_ivars();
};

}
