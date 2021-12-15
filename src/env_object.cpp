#include "natalie.hpp"
#include "natalie/env.hpp"
#include "tm/vector.hpp"

extern char **environ;

namespace Natalie {

Value EnvObject::inspect(Env *env) {
    HashObject *hash = new HashObject {};
    int i = 1;
    char *pair = *environ;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        size_t index = eq - pair;
        StringObject *name = new StringObject { pair };
        name->truncate(index);
        hash->put(env, name, new StringObject { getenv(name->c_str()) });
        pair = *(environ + i);
    }
    return hash->inspect(env);
}

Value EnvObject::ref(Env *env, Value name) {
    name->assert_type(env, Object::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringObject { value };
    } else {
        return NilObject::the();
    }
}

Value EnvObject::refeq(Env *env, Value name, Value value) {
    name->assert_type(env, Object::Type::String, "String");
    value->assert_type(env, Object::Type::String, "String");
    setenv(name->as_string()->c_str(), value->as_string()->c_str(), 1);
    return value;
}

}
