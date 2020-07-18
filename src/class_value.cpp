#include "natalie.hpp"

namespace Natalie {

ClassValue *ClassValue::subclass(Env *env, const char *name) {
    return subclass(env, name, m_object_type);
}

ClassValue *ClassValue::subclass(Env *env, const char *name, Type object_type) {
    ClassValue *subclass = new ClassValue { env, this->klass };
    subclass->env = Env::new_detatched_block_env(&this->env);
    if (m_singleton_class) {
        // TODO: what happens if the superclass gets a singleton_class later?
        subclass->set_singleton_class(m_singleton_class->subclass(env));
    }
    subclass->set_class_name(name);
    subclass->m_superclass = this;
    subclass->m_object_type = object_type;
    return subclass;
}

ClassValue *ClassValue::bootstrap_class_class(Env *env) {
    ClassValue *Class = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    Class->klass = Class;
    Class->set_class_name("Class");
    return Class;
}

ClassValue *ClassValue::bootstrap_basic_object(Env *env, ClassValue *Class) {
    ClassValue *BasicObject = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    BasicObject->klass = Class;
    BasicObject->set_class_name("BasicObject");
    return BasicObject;
}

}
