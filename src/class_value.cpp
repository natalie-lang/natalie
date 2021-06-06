#include "natalie.hpp"

namespace Natalie {

ClassValue *ClassValue::subclass(Env *env, const String *name, Type object_type) {
    ClassValue *subclass = new ClassValue { klass() };
    subclass->m_env = new Env {};
    if (singleton_class()) {
        auto singleton_name = String::format("#<Class:{}>", name);
        ClassValue *singleton = singleton_class()->subclass(env, singleton_name);
        subclass->set_singleton_class(singleton);
    }
    if (name)
        subclass->set_class_name(name);
    subclass->m_superclass = this;
    subclass->m_object_type = object_type;
    return subclass;
}

ClassValue *ClassValue::bootstrap_class_class(Env *env) {
    ClassValue *Class = new ClassValue {
        reinterpret_cast<ClassValue *>(-1)
    };
    Class->m_klass = Class;
    Class->set_class_name("Class");
    return Class;
}

ClassValue *ClassValue::bootstrap_basic_object(Env *env, ClassValue *Class) {
    ClassValue *BasicObject = new ClassValue {
        reinterpret_cast<ClassValue *>(-1)
    };
    BasicObject->m_klass = Class;
    BasicObject->m_superclass = nullptr;
    BasicObject->set_class_name("BasicObject");
    BasicObject->set_singleton_class(Class->subclass(env, "#<Class:BasicObject>"));
    return BasicObject;
}

}
