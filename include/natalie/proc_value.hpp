#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ProcValue : Value {
    enum class ProcType {
        Proc,
        Lambda
    };

    ProcValue(Env *env)
        : Value { Value::Type::Proc, NAT_OBJECT->const_get(env, "Proc", true)->as_class() } { }

    ProcValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Proc, klass } { }

    ProcValue(Env *env, Block *block, ProcType type = ProcType::Proc)
        : Value { Value::Type::Proc, NAT_OBJECT->const_get(env, "Proc", true)->as_class() }
        , m_block { block }
        , m_type { type } {
        assert(m_block);
    }

    Value *initialize(Env *, Block *);

    Block *block() { return m_block; }
    bool is_lambda() { return m_type == ProcType::Lambda; }

    virtual ProcValue *to_proc(Env *) override {
        // TODO: might need to return a copy with m_type set to Proc if this is a Lambda
        return this;
    }

    Value *call(Env *, ssize_t, Value **, Block *);

private:
    Block *m_block { nullptr };
    ProcType m_type { ProcType::Proc };
};

}
