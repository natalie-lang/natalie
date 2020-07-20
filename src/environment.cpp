#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include "natalie/env.hpp"
#include "natalie/vector.hpp"

extern char **environ;

namespace Natalie {

Value *ENV_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
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
    return Hash_inspect(env, hash, 0, nullptr, nullptr);
}

Value *ENV_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *name = args[0];
    NAT_ASSERT_TYPE(name, Value::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringValue { env, value };
    } else {
        return NAT_NIL;
    }
}

Value *ENV_refeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    Value *name = args[0];
    Value *value = args[1];
    NAT_ASSERT_TYPE(name, Value::Type::String, "String");
    NAT_ASSERT_TYPE(value, Value::Type::String, "String");
    setenv(name->as_string()->c_str(), value->as_string()->c_str(), 1);
    return value;
}

}
