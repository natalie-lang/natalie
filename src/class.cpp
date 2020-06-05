#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Class_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    ClassValue *superclass;
    if (argc == 1) {
        superclass = args[0]->as_class();
        if (NAT_TYPE(superclass) != Value::Type::Class) {
            NAT_RAISE(env, "TypeError", "superclass must be a Class (%s given)", NAT_OBJ_CLASS(superclass)->class_name);
        }
    } else {
        superclass = NAT_OBJECT;
    }
    Value *klass = superclass->subclass(env, nullptr);
    if (block) {
        Env e = Env::new_block_env(&block->env, env);
        block->fn(&e, klass, 0, NULL, NULL);
    }
    return klass;
}

Value *Class_superclass(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ClassValue *self = self_value->as_class();
    NAT_ASSERT_ARGC(0);
    if (self->superclass) {
        return self->superclass;
    } else {
        return NAT_NIL;
    }
}

}
