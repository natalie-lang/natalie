#include "natalie.hpp"

namespace Natalie {

Value ClassObject::initialize(Env *env, Value superclass, Block *block) {
    if (superclass) {
        if (!superclass->is_class()) {
            env->raise("TypeError", "superclass must be a Class ({} given)", superclass->klass()->class_name_or_blank());
        }
    } else {
        superclass = GlobalEnv::the()->Object();
    }
    superclass->as_class()->initialize_subclass(this, env, nullptr, superclass->as_class()->object_type());
    ModuleObject::initialize(env, block);
    return this;
}

ClassObject *ClassObject::subclass(Env *env, const String *name, Type object_type) {
    ClassObject *subclass = new ClassObject { klass() };
    initialize_subclass(subclass, env, name, object_type);
    return subclass;
}

void ClassObject::initialize_subclass(ClassObject *subclass, Env *env, const String *name, Type object_type) {
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
    subclass->m_is_initialized = true;
}

ClassObject *ClassObject::bootstrap_class_class(Env *env) {
    ClassObject *Class = new ClassObject {
        reinterpret_cast<ClassObject *>(-1)
    };
    Class->m_klass = Class;
    Class->m_object_type = Type::Class;
    Class->m_is_initialized = true;
    Class->set_class_name("Class");
    return Class;
}

ClassObject *ClassObject::bootstrap_basic_object(Env *env, ClassObject *Class) {
    ClassObject *BasicObject = new ClassObject {
        reinterpret_cast<ClassObject *>(-1)
    };
    BasicObject->m_klass = Class;
    BasicObject->m_superclass = nullptr;
    BasicObject->m_is_initialized = true;
    BasicObject->set_class_name("BasicObject");
    BasicObject->set_singleton_class(Class->subclass(env, "#<Class:BasicObject>"));
    return BasicObject;
}

}
