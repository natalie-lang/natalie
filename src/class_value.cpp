#include "natalie.hpp"

namespace Natalie {

ClassValue::ClassValue(Env *env)
    : ClassValue { env, NAT_OBJECT->const_get(env, "Class", true)->as_class() } { }

ClassValue *ClassValue::subclass(Env *env, const char *name) {
    ClassValue *subclass = new ClassValue { env, Value::Type::Class, this->klass };
    subclass->env = Env::new_detatched_block_env(&this->env);
    if (this->singleton_class) {
        // TODO: what happens if the superclass gets a singleton_class later?
        subclass->singleton_class = this->singleton_class->subclass(env, nullptr);
    }
    subclass->class_name = name ? strdup(name) : nullptr;
    subclass->superclass = this;
    return subclass;
}

ClassValue *ClassValue::bootstrap_class_class(Env *env) {
    ClassValue *Class = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    Class->klass = Class;
    Class->class_name = strdup("Class");
    return Class;
}

ClassValue *ClassValue::bootstrap_basic_object(Env *env, ClassValue *Class) {
    ClassValue *BasicObject = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    BasicObject->klass = Class;
    BasicObject->class_name = strdup("BasicObject");
    return BasicObject;
}

ClassValue::ClassValue(Env *env, ClassValue *klass)
    : ModuleValue { env, Value::Type::Class, klass } { }

}
