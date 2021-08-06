#pragma once

#include <assert.h>

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/module_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ClassValue : public ModuleValue {
public:
    ClassValue()
        : ClassValue { GlobalEnv::the()->Class() } { }

    ClassValue(ClassValue *klass)
        : ModuleValue { Value::Type::Class, klass } { }

    ClassValue *subclass(Env *env, const char *name) {
        return subclass(env, name, m_object_type);
    }

    ClassValue *subclass(Env *env, const char *name, Type object_type) {
        return subclass(env, new String(name), object_type);
    }

    ClassValue *subclass(Env *env, const String *name = nullptr) {
        return subclass(env, name, m_object_type);
    }

    ClassValue *subclass(Env *, const String *, Type);

    static ClassValue *bootstrap_class_class(Env *);
    static ClassValue *bootstrap_basic_object(Env *, ClassValue *);

    Type object_type() { return m_object_type; }

    ValuePtr initialize(Env *, ValuePtr, Block *);

    static ValuePtr new_method(Env *env, ValuePtr superclass, Block *block) {
        if (superclass) {
            if (!superclass->is_class()) {
                env->raise("TypeError", "superclass must be a Class ({} given)", superclass->klass()->class_name_or_blank());
            }
        } else {
            superclass = GlobalEnv::the()->Object();
        }
        ValuePtr klass = superclass->as_class()->subclass(env);
        if (block) {
            block->set_self(klass);
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
        }
        return klass;
    }

    bool is_singleton() { return m_is_singleton; }
    void set_is_singleton(bool is_singleton) { m_is_singleton = is_singleton; }

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (m_class_name)
            snprintf(buf, len, "<ClassValue %p name=%p>", this, m_class_name.value());
        else
            snprintf(buf, len, "<ClassValue %p name=(none)>", this);
    }

private:
    Type m_object_type { Type::Object };
    bool m_is_singleton { false };
};

}
