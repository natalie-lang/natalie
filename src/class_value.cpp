#include "natalie.hpp"

namespace Natalie {

ClassValue::ClassValue(Env *env)
    : ClassValue { env, const_get(env, NAT_OBJECT, "Class", true)->as_class() } { }

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

/* A note about bootstrapping...
 * The Value constructor has an invariant that all values must have a class (klass).
 * This works for all objects, except for the very first objects, Ruby's `Class` class
 * and `BasicObject` class. The following static method and private constructor are only
 * used during the bootstrap process, when our Ruby implementation is coming up, to build
 * these two classes. This method should not be used anywhere else!
 */

ClassValue *ClassValue::bootstrap_the_first_class(Env *env, ClassValue *klass) {
    ClassValue *new_class = new ClassValue {
        env,
        klass ? klass : reinterpret_cast<ClassValue *>(-1)
    };
    if (klass == nullptr) new_class->klass = new_class;
    return new_class;
}

ClassValue::ClassValue(Env *env, ClassValue *klass)
    : ModuleValue { env, Value::Type::Class, klass } { }

}
