#include "natalie.hpp"
#include "natalie/env.hpp"
#include "tm/vector.hpp"

extern char **environ;

namespace Natalie {

ValuePtr EnvObject::inspect(Env *env) {
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

ValuePtr EnvObject::ref(Env *env, ValuePtr name) {
    name->assert_type(env, Object::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringObject { value };
    } else {
        return NilObject::the();
    }
}

ValuePtr EnvObject::refeq(Env *env, ValuePtr name, ValuePtr value) {
    name->assert_type(env, Object::Type::String, "String");
    value->assert_type(env, Object::Type::String, "String");
    setenv(name->as_string()->c_str(), value->as_string()->c_str(), 1);
    return value;
}

}
