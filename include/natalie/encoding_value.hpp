#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/array_value.hpp"
#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

enum class Encoding {
    ASCII_8BIT = 1,
    UTF_8 = 2,
};

struct EncodingValue : Value {

    EncodingValue(Env *env);

    EncodingValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Encoding, klass } { }

    EncodingValue(Env *, Encoding, std::initializer_list<const char *>);

    Encoding num() { return m_num; }

    const StringValue *name() { return m_names[0]; }
    ValuePtr name(Env *);

    ArrayValue *names(Env *);

    ValuePtr inspect(Env *);

    static ArrayValue *list(Env *env);

    virtual void visit_children(Visitor &) override final;

private:
    Vector<StringValue *> m_names {};
    Encoding m_num;
};

EncodingValue *encoding(Env *env, Encoding num, ArrayValue *names);

}
