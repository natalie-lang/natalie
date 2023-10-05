#include "natalie.hpp"

namespace Natalie {

Value ClassObject::initialize(Env *env, Value superclass, Block *block) {
    if (!superclass)
        superclass = GlobalEnv::the()->Object();
    if (!superclass->is_class())
        env->raise("TypeError", "superclass must be an instance of Class (given an instance of {})", superclass->klass()->inspect_str());
    superclass->as_class()->initialize_subclass(this, env, "", superclass->as_class()->object_type());
    ModuleObject::initialize(env, block);
    return this;
}

ClassObject *ClassObject::subclass(Env *env, String name, Type object_type) {
    ClassObject *subclass = new ClassObject { klass() };
    initialize_subclass(subclass, env, name, object_type);
    return subclass;
}

void ClassObject::initialize_subclass(ClassObject *subclass, Env *env, String name, Optional<Type> object_type) {
    if (this == GlobalEnv::the()->Class())
        env->raise("TypeError", "can't make subclass of Class");
    if (m_is_singleton)
        env->raise("TypeError", "can't make subclass of singleton class");
    initialize_subclass_without_checks(subclass, env, name, object_type);
}

void ClassObject::initialize_subclass_without_checks(ClassObject *subclass, Env *env, const String name, Optional<Type> object_type) {
    subclass->m_env = new Env {};
    if (singleton_class()) {
        String singleton_name;
        if (!name.is_empty())
            singleton_name = String::format("#<Class:{}>", name);
        auto my_singleton_class = singleton_class();
        ClassObject *singleton = new ClassObject { my_singleton_class };
        my_singleton_class->initialize_subclass_without_checks(singleton, env, singleton_name, my_singleton_class->object_type());
        subclass->set_singleton_class(singleton);
    }
    if (!name.is_empty())
        subclass->set_class_name(name);
    subclass->m_superclass = this;
    if (!object_type)
        object_type = m_object_type;
    subclass->m_object_type = *object_type;
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
    auto singleton = new ClassObject { Class };
    Class->initialize_subclass_without_checks(singleton, env, "#<Class:BasicObject>");
    BasicObject->set_singleton_class(singleton);
    return BasicObject;
}

String ClassObject::backtrace_name() const {
    if (!m_class_name)
        return inspect_str();
    return String::format("<class:{}>", m_class_name.value());
}

}
