#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

struct Value {
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

    // TODO: don't need env here
    Value(Env *env, ClassValue *klass)
        : klass { klass } {
        assert(klass);
    }

    // TODO: don't need env here
    Value(Env *env, Type type, ClassValue *klass)
        : type { type }
        , klass { klass } {
        assert(klass);
    }

    Type type { Type::Object };
    ClassValue *klass { nullptr };

    ClassValue *singleton_class { nullptr };

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

    Value *ivar_get(Env *, const char *);
    Value *ivar_set(Env *, const char *, Value *);

    Value *cvar_get(Env *, const char *);
    Value *cvar_get_or_null(Env *, const char *);
    Value *cvar_set(Env *, const char *, Value *);

    virtual Value *const_get(Env *, const char *, bool);
    virtual Value *const_get_or_null(Env *, const char *, bool, bool);
    virtual Value *const_set(Env *, const char *, Value *);

private:
    void init_ivars();
};

}
