#include "natalie.hpp"
#include "natalie/env.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

extern char **environ;

namespace Natalie {

static Value env_size(Env *env, Value self, Args &&, Block *) {
    return self.send(env, "size"_s);
}

static inline StringObject *string_with_default_encoding(const char *str) {
    if (EncodingObject::default_internal())
        return StringObject::create(str, EncodingObject::default_internal());
    return StringObject::create(str);
}

static inline StringObject *string_with_default_encoding(const char *str, size_t len) {
    if (EncodingObject::default_internal())
        return StringObject::create(str, len, EncodingObject::default_internal());
    return StringObject::create(str, len);
}

Value EnvObject::inspect(Env *env) {
    return this->to_hash(env, nullptr).as_hash()->inspect(env);
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
        Value name = string_with_default_encoding(pair, index);
        Value value = string_with_default_encoding(getenv(name.as_string()->c_str()));
        if (block) {
            auto transformed = block->run(env, Args({ name, value }), nullptr);
            if (!transformed.is_array() && transformed.respond_to(env, "to_ary"_s))
                transformed = transformed.to_ary(env);
            if (!transformed.is_array())
                env->raise("TypeError", "wrong element type {} (expected array)", transformed.klass()->inspect_module());
            if (transformed.as_array()->size() != 2)
                env->raise("ArgumentError", "element has wrong array length (expected 2, was {})", transformed.as_array()->size());
            name = transformed.as_array()->at(0);
            value = transformed.as_array()->at(1);
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

Value EnvObject::delete_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "delete_if"_s }, size_block);
    }

    reject_in_place(env, block);
    return this;
}

Value EnvObject::delete_key(Env *env, Value name, Block *block) {
    auto namestr = name.to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        auto value_obj = StringObject::create(value);
        ::unsetenv(namestr->c_str());
        return value_obj;
    } else if (block) {
        return block->run(env, Args({ name }), nullptr);
    } else {
        return Value::nil();
    }
}

Value EnvObject::dup(Env *env) {
    env->raise("TypeError", "Cannot dup ENV, use ENV.to_h to get a copy of ENV as a hash");

    return Value::nil(); // No void return type
}

Value EnvObject::each(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashKey &node : *envhash.as_hash()) {
            auto name = node.key;
            auto value = node.val;
            block->run(env, Args({ name, value }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { *env, envhash.as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }
}

Value EnvObject::each_key(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashKey &node : *envhash.as_hash()) {
            auto name = node.key;
            block->run(env, Args({ name }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { *env, envhash.as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_key"_s }, size_block);
    }
}

Value EnvObject::each_value(Env *env, Block *block) {
    if (block) {
        auto envhash = to_hash(env, nullptr);
        for (HashKey &node : *envhash.as_hash()) {
            auto value = node.val;
            block->run(env, Args({ value }), nullptr);
        }
        return this;
    } else {
        auto envhash = to_hash(env, nullptr);
        Block *size_block = new Block { *env, envhash.as_hash(), HashObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_value"_s }, size_block);
    }
}

Value EnvObject::assoc(Env *env, Value name) {
    auto namestr = name.to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        StringObject *valuestr = StringObject::create(value);
        return ArrayObject::create({ namestr, valuestr });
    } else {
        return Value::nil();
    }
}

Value EnvObject::rassoc(Env *env, Value value) {
    if (!value.is_string() && value.respond_to(env, "to_str"_s))
        value = value.to_str(env);
    if (!value.is_string())
        return Value::nil();
    auto name = key(env, value);
    if (name.is_nil())
        return Value::nil();
    return ArrayObject::create({ name, value });
}

Value EnvObject::ref(Env *env, Value name) {
    auto namestr = name.to_str(env);
    char *value = getenv(namestr->c_str());
    if (value) {
        return string_with_default_encoding(value);
    } else {
        return Value::nil();
    }
}

Value EnvObject::except(Env *env, Args &&args) {
    auto result = to_hash(env, nullptr).as_hash();
    for (size_t i = 0; i < args.size(); i++) {
        result->remove(env, args[i]);
    }
    return result;
}

Value EnvObject::fetch(Env *env, Value name, Optional<Value> default_arg, Block *block) {
    name.assert_type(env, Object::Type::String, "String");
    char *value = getenv(name.as_string()->c_str());
    if (value) {
        return StringObject::create(value);
    } else if (block) {
        if (default_arg)
            env->warn("block supersedes default value argument");
        return block->run(env, Args({ name }), nullptr);
    } else if (default_arg) {
        return default_arg.value();
    } else {
        env->raise_key_error(this, name);
    }
}

Value EnvObject::refeq(Env *env, Value name, Value value) {
    auto namestr = name.to_str(env);
    if (value.is_nil()) {
        unsetenv(namestr->c_str());
    } else {
        auto valuestr = value.to_str(env);
        auto result = setenv(namestr->c_str(), valuestr->c_str(), 1);
        if (result == -1)
            env->raise_errno();
    }
    return value;
}

Value EnvObject::keep_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "keep_if"_s }, size_block);
    }

    select_in_place(env, block);
    return this;
}

Value EnvObject::key(Env *env, Value value) {
    value = value.to_str(env);
    const auto &needle = value.as_string()->string();

    size_t i = 1;
    char *pair = *environ;
    for (size_t i = 1; pair != nullptr; i++) {
        const char *eq = strchr(pair, '=');
        assert(eq);
        if (needle == eq + 1)
            return StringObject::create(pair, static_cast<size_t>(eq - pair));

        pair = *(environ + i);
    }

    return Value::nil();
}

Value EnvObject::keys(Env *env) {
    return to_hash(env, nullptr).as_hash()->keys(env);
}

bool EnvObject::has_key(Env *env, Value name) {
    auto namestr = name.to_str(env);
    char *value = getenv(namestr->c_str());
    return (value != NULL);
}

Value EnvObject::has_value(Env *env, Value name) {
    if (!name.respond_to(env, "to_str"_s))
        return Value::nil();
    name = name.to_str(env);
    if (to_hash(env, nullptr).as_hash()->has_value(env, name))
        return Value::True();
    return Value::False();
}

Value EnvObject::to_s() const {
    return StringObject::create("ENV");
}

Value EnvObject::rehash() const {
    return Value::nil();
}

// Sadly, clearenv() is not available on some OS's
// but is probably more optimal than this solution
Value EnvObject::clear(Env *env) {
    auto envhash = to_hash(env, nullptr);
    for (HashKey &node : *envhash.as_hash()) {
        ::unsetenv(node.key.as_string()->c_str());
    }
    return this;
}

Value EnvObject::clone(Env *env) {
    env->raise("TypeError", "Cannot clone ENV, use ENV.to_h to get a copy of ENV as a hash");
}

Value EnvObject::reject(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "reject"_s }, size_block);
    }

    return to_hash(env, nullptr).as_hash()->delete_if(env, block);
}

Value EnvObject::reject_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "reject!"_s }, size_block);
    }

    auto hash = to_hash(env, nullptr).as_hash();
    auto keys = hash->keys(env).as_array();
    if (hash->send(env, "reject!"_s, {}, block).is_nil())
        // No changes
        return Value::nil();

    for (size_t i = 0; i < keys->size(); i++) {
        auto key = keys->at(i);
        if (!hash->has_key(env, key))
            unsetenv(key.as_string()->c_str());
    }
    return this;
}

Value EnvObject::replace(Env *env, Value hash) {
    hash.assert_type(env, Object::Type::Hash, "Hash");
    for (HashKey &node : *hash.as_hash()) {
        node.key.assert_type(env, Object::Type::String, "String");
        node.val.assert_type(env, Object::Type::String, "String");
    }
    clear(env);
    for (HashKey &node : *hash.as_hash()) {
        auto result = ::setenv(node.key.as_string()->c_str(), node.val.as_string()->c_str(), 1);
        if (result == -1)
            env->raise_errno();
    }
    return this;
}

Value EnvObject::select(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "select"_s }, size_block);
    }

    return to_hash(env, nullptr).as_hash()->keep_if(env, block);
}

Value EnvObject::select_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, env_size, 0 };
        return send(env, "enum_for"_s, { "select!"_s }, size_block);
    }

    auto hash = to_hash(env, nullptr).as_hash();
    auto keys = hash->keys(env).as_array();
    if (hash->send(env, "select!"_s, {}, block).is_nil())
        // No changes
        return Value::nil();

    for (size_t i = 0; i < keys->size(); i++) {
        auto key = keys->at(i);
        if (!hash->has_key(env, key))
            unsetenv(key.as_string()->c_str());
    }
    return this;
}

Value EnvObject::shift() {
    if (!environ)
        return Value::nil();

    char *pair = *environ;
    if (!pair)
        return Value::nil();

    char *eq = strchr(pair, '=');
    assert(eq);
    auto name = string_with_default_encoding(pair, static_cast<size_t>(eq - pair));
    auto value = string_with_default_encoding(getenv(name->c_str()));
    unsetenv(name->c_str());
    return ArrayObject::create({ name, value });
}

Value EnvObject::invert(Env *env) {
    return to_hash(env, nullptr).send(env, "invert"_s);
}

bool EnvObject::is_empty() const {
    if (!environ) return true;
    char *pair = *environ;
    if (!pair) return true;
    return false;
}

Value EnvObject::slice(Env *env, Args &&args) {
    if (args.has_keyword_hash())
        env->raise("TypeError", "no implicit conversion of Hash into String");

    auto result = new HashObject;
    for (size_t i = 0; i < args.size(); i++) {
        auto name = args[i];
        auto namestr = name.to_str(env);

        const char *value = getenv(namestr->c_str());
        if (value != nullptr) {
            result->put(env, name, StringObject::create(value));
        }
    }
    return result;
}

Value EnvObject::update(Env *env, Args &&args, Block *block) {
    for (size_t i = 0; i < args.size(); i++) {
        auto h = args[i];

        if (!h.is_hash() && h.respond_to(env, "to_hash"_s))
            h = h.send(env, "to_hash"_s);

        h.assert_type(env, Object::Type::Hash, "Hash");

        for (auto node : *h.as_hash()) {
            auto old_value = ref(env, node.key);
            auto new_value = node.val;
            if (!old_value.is_nil()) {
                if (block) {
                    Value args[3] = { node.key, old_value, new_value };
                    new_value = block->run(env, Args(3, args), nullptr);
                }
            }
            refeq(env, node.key, new_value);
        }
    }
    return this;
}

Value EnvObject::values(Env *env) {
    return to_hash(env, nullptr).as_hash()->values(env);
}

Value EnvObject::values_at(Env *env, Args &&args) {
    auto result = ArrayObject::create(args.size());
    for (size_t i = 0; i < args.size(); i++) {
        result->push(ref(env, args[i]));
    }
    return result;
}

}
