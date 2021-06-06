#include "natalie.hpp"
#include "natalie/env.hpp"
#include "tm/vector.hpp"

extern char **environ;

namespace Natalie {

ValuePtr EnvValue::inspect(Env *env) {
    HashValue *hash = new HashValue {};
    int i = 1;
    char *pair = *environ;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        size_t index = eq - pair;
        StringValue *name = new StringValue { pair };
        name->truncate(index);
        hash->put(env, name, new StringValue { getenv(name->c_str()) });
        pair = *(environ + i);
    }
    return hash->inspect(env);
}

ValuePtr EnvValue::ref(Env *env, ValuePtr name) {
    name->assert_type(env, Value::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringValue { value };
    } else {
        return env->nil_obj();
    }
}

ValuePtr EnvValue::refeq(Env *env, ValuePtr name, ValuePtr value) {
    name->assert_type(env, Value::Type::String, "String");
    value->assert_type(env, Value::Type::String, "String");
    setenv(name->as_string()->c_str(), value->as_string()->c_str(), 1);
    return value;
}

}
