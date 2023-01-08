#include "natalie.hpp"
#include "natalie/env.hpp"
#include "tm/vector.hpp"

extern char **environ;

namespace Natalie {

Value EnvObject::inspect(Env *env) {
    return this->to_hash(env)->as_hash()->inspect(env);
}

Value EnvObject::to_hash(Env *env) {
    HashObject *hash = new HashObject {};
    size_t i = 1;
    char *pair = *environ;
    if (!pair) return hash;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        size_t index = eq - pair;
        StringObject *name = new StringObject { pair };
        name->truncate(index);
        hash->put(env, name, new StringObject { getenv(name->c_str()) });
        pair = *(environ + i);
    }
    return hash;
}

Value EnvObject::size(Env *env) const {
    size_t idx = 0;
    if (!environ) return Value::integer(0);
    char *pair = *environ;
    while (pair) {
        idx++;
        pair = *(environ + idx);
    }
    return IntegerObject::from_size_t(env, idx);
}

Value EnvObject::delete_key(Env *env, Value name, Block *block) {
    name->assert_type(env, Object::Type::String, "String");
    auto namestr = name->as_string()->c_str();
    char *value = getenv(namestr);
    if (value) {
        auto value_obj = new StringObject { value };
        ::unsetenv(namestr);
        return value_obj;
    } else if (block) {
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name }), nullptr);
    } else {
        return NilObject::the();
    }
}

Value EnvObject::each(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env);
        for (HashObject::Key &node : *envhash->as_hash()) {
            auto name = node.key;
            auto value = node.val;
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name, value }), nullptr);
        }
        return this;
    } else {
        // NATFIXME: return an enumerator
        return NilObject::the();
    }
}

Value EnvObject::assoc(Env *env, Value name) {
    StringObject *namestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        StringObject *valuestr = new StringObject { value };
        return new ArrayObject { { namestr, valuestr } };
    } else {
        return NilObject::the();
    }
}
Value EnvObject::ref(Env *env, Value name) {
    StringObject *namestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        return new StringObject { value };
    } else {
        return NilObject::the();
    }
}

Value EnvObject::fetch(Env *env, Value name, Value default_value, Block *block) {
    name->assert_type(env, Object::Type::String, "String");
    char *value = getenv(name->as_string()->c_str());
    if (value) {
        return new StringObject { value };
    } else if (block) {
        if (default_value)
            env->warn("block supersedes default value argument");
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name }), nullptr);
    } else if (default_value) {
        return default_value;
    } else {
        env->raise_key_error(this, name);
    }
}

Value EnvObject::refeq(Env *env, Value name, Value value) {
    StringObject *namestr, *valuestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    if (value->is_nil()) {
        unsetenv(namestr->c_str());
    } else {
        valuestr = value->is_string() ? value->as_string() : value->to_str(env);
        auto result = setenv(namestr->c_str(), valuestr->c_str(), 1);
        if (result == -1)
            env->raise_errno();
    }
    return value;
}

Value EnvObject::keys(Env *env) {
    return to_hash(env)->as_hash()->keys(env);
}

bool EnvObject::has_key(Env *env, Value name) {
    StringObject *namestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    return (value != NULL);
}

Value EnvObject::to_s() const {
    return new StringObject { "ENV" };
}

Value EnvObject::rehash() const {
    return NilObject::the();
}

// Sadly, clearenv() is not available on some OS's
// but is probably more optimal than this solution
Value EnvObject::clear(Env *env) {
    auto envhash = to_hash(env);
    for (HashObject::Key &node : *envhash->as_hash()) {
        ::unsetenv(node.key->as_string()->c_str());
    }
    return this;
}

Value EnvObject::replace(Env *env, Value hash) {
    hash->assert_type(env, Object::Type::Hash, "Hash");
    for (HashObject::Key &node : *hash->as_hash()) {
        node.key->assert_type(env, Object::Type::String, "String");
        node.val->assert_type(env, Object::Type::String, "String");
    }
    clear(env);
    for (HashObject::Key &node : *hash->as_hash()) {
        auto result = ::setenv(node.key->as_string()->c_str(), node.val->as_string()->c_str(), 1);
        if (result == -1)
            env->raise_errno();
    }
    return this;
}

bool EnvObject::is_empty() const {
    size_t idx = 0;
    if (!environ) return true;
    char *pair = *environ;
    if (!pair) return true;
    return false;
}

Value EnvObject::update(Env *env, Args args, Block *block) {
    for (size_t i = 0; i < args.size(); i++) {
        auto h = args[i];

        if (!h->is_hash() && h->respond_to(env, "to_hash"_s))
            h = h->send(env, "to_hash"_s);

        h->assert_type(env, Object::Type::Hash, "Hash");

        for (auto node : *h->as_hash()) {
            auto old_value = ref(env, node.key);
            auto new_value = node.val;
            if (!old_value->is_nil()) {
                if (block) {
                    Value args[3] = { node.key, old_value, new_value };
                    new_value = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, Args(3, args), nullptr);
                }
            }
            refeq(env, node.key, new_value);
        }
    }
    return this;
}

}
