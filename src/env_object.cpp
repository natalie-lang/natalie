#include "natalie.hpp"
#include "natalie/env.hpp"
#include "tm/vector.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

extern char **environ;

namespace Natalie {

Value EnvObject::inspect(Env *env) {
    return this->to_hash(env, nullptr)->as_hash()->inspect(env);
}

Value EnvObject::to_hash(Env *env, Block *block) {
    HashObject *hash = new HashObject {};
    size_t i = 1;
    char *pair = *environ;
    if (!pair) return hash;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        size_t index = eq - pair;
        Value name = new StringObject { pair };
        name->as_string()->truncate(index);
        Value value = new StringObject { getenv(name->as_string()->c_str()) };
        if (block) {
            auto transformed = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name, value }), nullptr);
            if (!transformed->is_array() && transformed->respond_to(env, "to_ary"_s))
                transformed = transformed->send(env, "to_ary"_s);
            if (!transformed->is_array())
                env->raise("TypeError", "wrong element type {} (expected array)", transformed->klass()->inspect_str());
            if (transformed->as_array()->size() != 2)
                env->raise("ArgumentError", "element has wrong array length (expected 2, was {})", transformed->as_array()->size());
            name = transformed->as_array()->at(0);
            value = transformed->as_array()->at(1);
        }
        hash->put(env, name, value);
        pair = *(environ + i);
    }
    return hash;
}

size_t EnvObject::size() const {
    size_t idx = 0;
    if (!environ) return 0;
    char *pair = *environ;
    while (pair) {
        idx++;
        pair = *(environ + idx);
    }
    return idx;
}

Value EnvObject::delete_key(Env *env, Value name, Block *block) {
    auto namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        auto value_obj = new StringObject { value };
        ::unsetenv(namestr->c_str());
        return value_obj;
    } else if (block) {
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name }), nullptr);
    } else {
        return NilObject::the();
    }
}

Value EnvObject::dup(Env *env) {
    env->raise("TypeError", "Cannot dup ENV, use ENV.to_h to get a copy of ENV as a hash");

    return NilObject::the(); // No void return type
}

Value EnvObject::each(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashObject::Key &node : *envhash->as_hash()) {
            auto name = node.key;
            auto value = node.val;
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name, value }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { env, envhash->as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }
}

Value EnvObject::each_key(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashObject::Key &node : *envhash->as_hash()) {
            auto name = node.key;
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ name }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { env, envhash->as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_key"_s }, size_block);
    }
}

Value EnvObject::each_value(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashObject::Key &node : *envhash->as_hash()) {
            auto value = node.val;
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ value }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { env, envhash->as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_value"_s }, size_block);
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

Value EnvObject::rassoc(Env *env, Value value) {
    if (!value->is_string() && value->respond_to(env, "to_str"_s))
        value = value->to_str(env);
    if (!value->is_string())
        return NilObject::the();
    auto name = key(env, value);
    if (name->is_nil())
        return NilObject::the();
    return new ArrayObject { name, value };
}

Value EnvObject::ref(Env *env, Value name) {
    StringObject *namestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        if (EncodingObject::default_internal())
            return new StringObject { value, EncodingObject::default_internal() };
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

Value EnvObject::key(Env *env, Value value) {
    if (!value->is_string() && value->respond_to(env, "to_str"_s))
        value = value->send(env, "to_str"_s);
    value->assert_type(env, Object::Type::String, "String");

    const auto &needle = value->as_string()->string();

    size_t i = 1;
    char *pair = *environ;
    for (size_t i = 1; pair != nullptr; i++) {
        const char *eq = strchr(pair, '=');
        assert(eq);
        if (needle == eq + 1)
            return new StringObject { pair, static_cast<size_t>(eq - pair) };

        pair = *(environ + i);
    }

    return NilObject::the();
}

Value EnvObject::keys(Env *env) {
    return to_hash(env, nullptr)->as_hash()->keys(env);
}

bool EnvObject::has_key(Env *env, Value name) {
    StringObject *namestr;
    namestr = name->is_string() ? name->as_string() : name->to_str(env);
    char *value = getenv(namestr->c_str());
    return (value != NULL);
}

Value EnvObject::has_value(Env *env, Value name) {
    if (!name->is_string()) {
        if (!name->respond_to(env, "to_str"_s))
            return NilObject::the();
        name = name->send(env, "to_str"_s);
    }
    if (to_hash(env, nullptr)->as_hash()->has_value(env, name))
        return TrueObject::the();
    return FalseObject::the();
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
    auto envhash = to_hash(env, nullptr);
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

Value EnvObject::shift() {
    if (!environ)
        return NilObject::the();

    char *pair = *environ;
    if (!pair)
        return NilObject::the();

    char *eq = strchr(pair, '=');
    assert(eq);
    auto name = new StringObject { pair, static_cast<size_t>(eq - pair) };
    auto value = new StringObject { getenv(name->c_str()) };
    unsetenv(name->c_str());
    return new ArrayObject { name, value };
}

Value EnvObject::invert(Env *env) {
    return to_hash(env, nullptr)->send(env, "invert"_s);
}

bool EnvObject::is_empty() const {
    if (!environ) return true;
    char *pair = *environ;
    if (!pair) return true;
    return false;
}

Value EnvObject::slice(Env *env, Args args) {
    if (args.has_keyword_hash())
        env->raise("TypeError", "no implicit conversion of Hash into String");

    auto result = new HashObject;
    for (size_t i = 0; i < args.size(); i++) {
        auto name = args[i];
        auto namestr = name->is_string() ? name->as_string() : name->to_str(env);

        const char *value = getenv(namestr->c_str());
        if (value != nullptr) {
            result->put(env, name, new StringObject { value });
        }
    }
    return result;
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

Value EnvObject::values(Env *env) {
    return to_hash(env, nullptr)->as_hash()->values(env);
}

Value EnvObject::values_at(Env *env, Args args) {
    auto result = new ArrayObject { args.size() };
    for (size_t i = 0; i < args.size(); i++) {
        result->push(ref(env, args[i]));
    }
    return result;
}

}
