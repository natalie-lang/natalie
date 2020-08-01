#include "natalie.hpp"
#include "natalie/env.hpp"
#include "natalie/vector.hpp"

extern char **environ;

namespace Natalie {

Value *EnvValue::inspect(Env *env) {
    HashValue *hash = new HashValue { env };
    int i = 1;
    char *pair = *environ;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        ssize_t index = eq - pair;
        StringValue *name = new StringValue { env, pair };
        name->truncate(index);
        hash->put(env, name, new StringValue { env, getenv(name->c_str()) });
        pair = *(environ + i);
    }
    return hash->inspect(env);
}

Value *EnvValue::ref(Env *env, Value *name) {
    NAT_ASSERT_TYPE(name, Value::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringValue { env, value };
    } else {
        return env->nil_obj();
    }
}

Value *EnvValue::refeq(Env *env, Value *name, Value *value) {
    NAT_ASSERT_TYPE(name, Value::Type::String, "String");
    NAT_ASSERT_TYPE(value, Value::Type::String, "String");
    setenv(name->as_string()->c_str(), value->as_string()->c_str(), 1);
    return value;
}

}
