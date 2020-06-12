#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include "natalie/env.hpp"
#include "natalie/vector.hpp"

extern char **environ;

namespace Natalie {

Value *Env::var_get(const char *key, ssize_t index) {
    if (index >= this->vars->size()) {
        printf("Trying to get variable `%s' at index %zu which is not set.\n", key, index);
        abort();
    }
    Value *val = (*this->vars)[index];
    if (val) {
        return val;
    } else {
        Env *env = this;
        return NAT_NIL;
    }
}

Value *Env::var_set(const char *name, ssize_t index, bool allocate, Value *val) {
    ssize_t needed = index + 1;
    ssize_t current_size = this->vars ? this->vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!this->vars) {
                this->vars = new Vector<Value *> { needed };
            } else {
                this->vars->set_size(needed);
            }
        } else {
            printf("Tried to set a variable without first allocating space for it.\n");
            abort();
        }
    }
    (*this->vars)[index] = val;
    return val;
}

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
    return Hash_inspect(env, hash, 0, NULL, NULL);
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
