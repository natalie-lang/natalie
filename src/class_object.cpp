#include "natalie.hpp"

namespace Natalie {

ClassObject *ClassObject::subclass(Env *env, const String *name, Type object_type) {
    ClassObject *subclass = new ClassObject { klass() };
    subclass->m_env = new Env {};
    if (singleton_class()) {
        auto singleton_name = String::format("#<Class:{}>", name);
        ClassObject *singleton = singleton_class()->subclass(env, singleton_name);
        subclass->set_singleton_class(singleton);
    }
    if (name)
        subclass->set_class_name(name);
    subclass->m_superclass = this;
    subclass->m_object_type = object_type;
    return subclass;
}

ClassObject *ClassObject::bootstrap_class_class(Env *env) {
    ClassObject *Class = new ClassObject {
        reinterpret_cast<ClassObject *>(-1)
    };
    Class->m_klass = Class;
    Class->set_class_name("Class");
    return Class;
}

ClassObject *ClassObject::bootstrap_basic_object(Env *env, ClassObject *Class) {
    ClassObject *BasicObject = new ClassObject {
        reinterpret_cast<ClassObject *>(-1)
    };
    BasicObject->m_klass = Class;
    BasicObject->m_superclass = nullptr;
    BasicObject->set_class_name("BasicObject");
    BasicObject->set_singleton_class(Class->subclass(env, "#<Class:BasicObject>"));
    return BasicObject;
}

}
