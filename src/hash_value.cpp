#include "natalie.hpp"
#include "tm/recursion_guard.hpp"
#include "tm/vector.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
size_t HashValue::hash(const void *key) {
    return static_cast<const HashValue::Key *>(key)->hash;
}

// this is used by the hashmap library to compare keys
bool HashValue::compare(const void *a, const void *b, void *env) {
    assert(env);
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;

    if (a_p->key->object_id() == b_p->key->object_id() && a_p->hash == b_p->hash)
        return true;

    return a_p->key.send((Env *)env, SymbolValue::intern("eql?"), { b_p->key })->is_truthy();
}

ValuePtr HashValue::compare_by_identity(Env *env) {
    assert_not_frozen(env);
    this->m_is_comparing_by_identity = true;
    this->rehash(env);
    return this;
}

ValuePtr HashValue::is_comparing_by_identity() {
    if (m_is_comparing_by_identity) {
        return TrueValue::the();
    } else {
        return FalseValue::the();
    }
}

ValuePtr HashValue::get(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = generate_key_hash(env, key);
    return m_hashmap.get(&key_container, env);
}

nat_int_t HashValue::generate_key_hash(Env *env, ValuePtr key) {
    if (m_is_comparing_by_identity) {
        return TM::Hashmap<void *>::hash_ptr(key.value());
    } else {
        return key.send(env, SymbolValue::intern("hash"))->as_integer()->to_nat_int_t();
    }
}

ValuePtr HashValue::get_default(Env *env, ValuePtr key) {
    if (m_default_proc) {
        if (!key)
            return NilValue::the();
        ValuePtr args[2] = { this, key };
        return m_default_proc->call(env, 2, args, nullptr);
    } else {
        return m_default_value;
    }
}

ValuePtr HashValue::set_default(Env *env, ValuePtr value) {
    assert_not_frozen(env);
    m_default_value = value;
    m_default_proc = nullptr;
    return value;
}

void HashValue::put(Env *env, ValuePtr key, ValuePtr val) {
    assert_not_frozen(env);
    Key key_container;
    if (!m_is_comparing_by_identity && key->is_string() && !key->is_frozen()) {
        key = key->as_string()->dup(env);
    }
    key_container.key = key;

    auto hash = generate_key_hash(env, key);
    key_container.hash = hash;
    auto entry = m_hashmap.find_item(&key_container, hash, env);
    if (entry) {
        ((Key *)entry->key)->val = val;
        entry->value = val.value();
    } else {
        if (m_is_iterating) {
            env->raise("RuntimeError", "can't add a new key into hash during iteration");
        }
        auto *key_container = key_list_append(env, key, hash, val);
        m_hashmap.put(key_container, val.value(), env);
    }
}

ValuePtr HashValue::remove(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    auto hash = generate_key_hash(env, key);
    key_container.hash = hash;
    auto entry = m_hashmap.find_item(&key_container, hash, env);
    if (entry) {
        key_list_remove_node((Key *)entry->key);
        auto val = (Value *)entry->value;
        m_hashmap.remove(&key_container, env);
        return val;
    } else {
        return nullptr;
    }
}

ValuePtr HashValue::clear(Env *env) {
    assert_not_frozen(env);
    m_hashmap.clear();
    m_key_list = nullptr;
    m_is_iterating = false;
    return this;
}

ValuePtr HashValue::default_proc(Env *env) {
    return m_default_proc;
}

ValuePtr HashValue::set_default_proc(Env *env, ValuePtr value) {
    assert_not_frozen(env);
    if (value == NilValue::the()) {
        m_default_proc = nullptr;
        return value;
    }
    auto to_proc = SymbolValue::intern("to_proc");
    auto to_proc_value = value;
    if (value->respond_to(env, to_proc))
        to_proc_value = value.send(env, to_proc);
    to_proc_value->assert_type(env, Type::Proc, "Proc");
    auto proc = to_proc_value->as_proc();
    auto arity = proc->arity();
    if (proc->is_lambda() && arity != 2)
        env->raise("TypeError", "default_proc takes two arguments (2 for {})", (long long)arity);
    m_default_proc = proc;
    return value;
}

HashValue::Key *HashValue::key_list_append(Env *env, ValuePtr key, nat_int_t hash, ValuePtr val) {
    if (m_key_list) {
        Key *first = m_key_list;
        Key *last = m_key_list->prev;
        Key *new_last = new Key {};
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->hash = hash;
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        Key *node = new Key {};
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        node->hash = hash;
        node->removed = false;
        m_key_list = node;
        return node;
    }
}

void HashValue::key_list_remove_node(Key *node) {
    Key *prev = node->prev;
    Key *next = node->next;
    // <prev> <-> <node> <-> <next>
    if (node == next) {
        // <node> -|
        // ^_______|
        node->prev = nullptr;
        node->next = nullptr;
        node->removed = true;
        m_key_list = nullptr;
        return;
    } else if (m_key_list == node) {
        // starting point is the node to be removed, so shift them forward by one
        m_key_list = next;
    }
    // remove the node
    node->removed = true;
    prev->next = next;
    next->prev = prev;
}

ValuePtr HashValue::initialize(Env *env, ValuePtr default_value, Block *block) {
    assert_not_frozen(env);

    if (block) {
        if (default_value) {
            env->raise("ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_proc(new ProcValue { block });
    } else {
        if (!default_value) {
            default_value = NilValue::the();
        }

        set_default(env, default_value);
    }
    return this;
}

// Hash[]
ValuePtr HashValue::square_new(Env *env, size_t argc, ValuePtr *args, ClassValue *klass) {
    if (argc == 0) {
        return new HashValue { klass };
    } else if (argc == 1) {
        ValuePtr value = args[0];
        if (!value->is_hash() && value->respond_to(env, SymbolValue::intern("to_hash")))
            value = value.send(env, SymbolValue::intern("to_hash"));
        if (value->is_hash()) {
            auto hash = new HashValue { env, *value->as_hash() };
            hash->m_klass = klass;
            return hash;
        } else {
            if (!value->is_array() && value->respond_to(env, SymbolValue::intern("to_ary")))
                value = value.send(env, SymbolValue::intern("to_ary"));
            if (value->is_array()) {
                HashValue *hash = new HashValue { klass };
                for (auto &pair : *value->as_array()) {
                    if (pair->type() != Value::Type::Array) {
                        env->raise("ArgumentError", "wrong element in array to Hash[]");
                    }
                    size_t size = pair->as_array()->size();
                    if (size < 1 || size > 2) {
                        env->raise("ArgumentError", "invalid number of elements ({} for 1..2)", size);
                    }
                    ValuePtr key = (*pair->as_array())[0];
                    ValuePtr value = size == 1 ? NilValue::the() : (*pair->as_array())[1];
                    hash->put(env, key, value);
                }
                return hash;
            }
        }
    }
    if (argc % 2 != 0) {
        env->raise("ArgumentError", "odd number of arguments for Hash");
    }
    HashValue *hash = new HashValue { klass };
    for (size_t i = 0; i < argc; i += 2) {
        ValuePtr key = args[i];
        ValuePtr value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

ValuePtr HashValue::inspect(Env *env) {
    TM::RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return new StringValue("{...}");
        StringValue *out = new StringValue { "{" };
        size_t last_index = size() - 1;
        size_t index = 0;

        auto to_s = [env](ValuePtr obj) {
            if (obj->is_string())
                return obj->as_string();
            if (obj->respond_to(env, SymbolValue::intern("to_s")))
                obj = obj->send(env, SymbolValue::intern("to_s"));
            else
                obj = new StringValue("?");
            if (!obj->is_string())
                obj = StringValue::format(env, "#<{}:{}>", obj->klass()->class_name_or_blank(), int_to_hex_string(obj->object_id(), false));
            return obj->as_string();
        };

        for (HashValue::Key &node : *this) {
            StringValue *key_repr = to_s(node.key.send(env, SymbolValue::intern("inspect")));
            out->append(env, key_repr);
            out->append(env, "=>");
            StringValue *val_repr = to_s(node.val.send(env, SymbolValue::intern("inspect")));
            out->append(env, val_repr);
            if (index < last_index) {
                out->append(env, ", ");
            }
            index++;
        }

        out->append_char(env, '}');
        return out;
    });
}

ValuePtr HashValue::ref(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return val;
    } else {
        return send(env, SymbolValue::intern("default"), { key });
    }
}

ValuePtr HashValue::refeq(Env *env, ValuePtr key, ValuePtr val) {
    put(env, key, val);
    return val;
}

ValuePtr HashValue::rehash(Env *env) {
    assert_not_frozen(env);

    auto copy = new HashValue { env, *this };
    HashValue::operator=(std::move(*copy));

    return this;
}

ValuePtr HashValue::replace(Env *env, ValuePtr other) {
    if (!other->is_hash() && other->respond_to(env, SymbolValue::intern("to_hash")))
        other = other->send(env, SymbolValue::intern("to_hash"));
    other->assert_type(env, Type::Hash, "Hash");

    auto other_hash = other->as_hash();

    clear(env);
    for (auto node : *other_hash) {
        put(env, node.key, node.val);
    }
    m_default_value = other_hash->m_default_value;
    m_default_proc = other_hash->m_default_proc;
    return this;
}

ValuePtr HashValue::delete_if(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("delete_if") });

    assert_not_frozen(env);
    for (auto &node : *this) {
        ValuePtr args[2] = { node.key, node.val };
        if (NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr)->is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

ValuePtr HashValue::delete_key(Env *env, ValuePtr key, Block *block) {
    assert_not_frozen(env);
    ValuePtr val = remove(env, key);
    if (val)
        return val;
    else if (block)
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 0, nullptr, nullptr);
    else
        return NilValue::the();
}

ValuePtr HashValue::dig(Env *env, size_t argc, ValuePtr *args) {
    env->ensure_argc_at_least(argc, 1);
    auto dig = SymbolValue::intern("dig");
    ValuePtr val = ref(env, args[0]);
    if (argc == 1)
        return val;

    if (val == NilValue::the())
        return val;

    if (!val->respond_to(env, dig))
        env->raise("TypeError", "{} does not have #dig method", val->klass()->class_name_or_blank());

    return val.send(env, dig, argc - 1, args + 1);
}

ValuePtr HashValue::size(Env *env) {
    return IntegerValue::from_size_t(env, size());
}

bool HashValue::eq(Env *env, ValuePtr other_value, SymbolValue *method_name) {
    if (!other_value->is_hash())
        return false;

    HashValue *other = other_value->as_hash();
    if (size() != other->size())
        return false;

    ValuePtr other_val;
    for (HashValue::Key &node : *this) {
        other_val = other->get(env, node.key);
        if (!other_val)
            return false;

        if (node.val.value() == other_val.value())
            continue;

        if (node.val.send(env, method_name, { other_val })->is_falsey())
            return false;
    }

    return true;
}

bool HashValue::eq(Env *env, ValuePtr other_value) {
    return eq(env, other_value, SymbolValue::intern("=="));
}

bool HashValue::eql(Env *env, ValuePtr other_value) {
    return eq(env, other_value, SymbolValue::intern("eql?"));
}

bool HashValue::gte(Env *env, ValuePtr other) {
    if (!other->is_hash() && other->respond_to_method(env, SymbolValue::intern("to_hash")))
        other = other->send(env, SymbolValue::intern("to_hash"));

    other->assert_type(env, Value::Type::Hash, "Hash");
    auto other_hash = other->as_hash();

    for (auto &node : *other_hash) {
        ValuePtr value = get(env, node.key);
        if (!value || value.send(env, SymbolValue::intern("=="), { node.val })->is_false()) {
            return false;
        }
    }
    return true;
}

bool HashValue::gt(Env *env, ValuePtr other) {
    if (!other->is_hash() && other->respond_to_method(env, SymbolValue::intern("to_hash")))
        other = other->send(env, SymbolValue::intern("to_hash"));

    other->assert_type(env, Value::Type::Hash, "Hash");

    return gte(env, other) && other->as_hash()->size() != size();
}

bool HashValue::lte(Env *env, ValuePtr other) {
    if (!other->is_hash() && other->respond_to_method(env, SymbolValue::intern("to_hash")))
        other = other->send(env, SymbolValue::intern("to_hash"));

    other->assert_type(env, Value::Type::Hash, "Hash");
    auto other_hash = other->as_hash();

    for (auto &node : *this) {
        ValuePtr value = other_hash->get(env, node.key);
        if (!value || value.send(env, SymbolValue::intern("=="), { node.val })->is_false()) {
            return false;
        }
    }
    return true;
}

bool HashValue::lt(Env *env, ValuePtr other) {
    if (!other->is_hash() && other->respond_to_method(env, SymbolValue::intern("to_hash")))
        other = other->send(env, SymbolValue::intern("to_hash"));

    other->assert_type(env, Value::Type::Hash, "Hash");

    return lte(env, other) && other->as_hash()->size() != size();
}

ValuePtr HashValue::each(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each") });

    ValuePtr block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (HashValue::Key &node : *this) {
        auto ary = new ArrayValue { { node.key, node.val } };
        ValuePtr block_args[1] = { ary };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, block_args, nullptr);
    }
    return this;
}

ValuePtr HashValue::except(Env *env, size_t argc, ValuePtr *args) {
    HashValue *new_hash = new HashValue {};
    for (auto &node : *this) {
        new_hash->put(env, node.key, node.val);
    }

    for (size_t i = 0; i < argc; i++) {
        new_hash->remove(env, args[i]);
    }
    return new_hash;
}

ValuePtr HashValue::fetch(Env *env, ValuePtr key, ValuePtr default_value, Block *block) {
    ValuePtr value = get(env, key);
    if (!value) {
        if (block) {
            if (default_value)
                env->warn("block supersedes default value argument");

            value = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &key, nullptr);
        } else if (default_value) {
            value = default_value;
        } else {
            env->raise_key_error(this, key);
        }
    }
    return value;
}

ValuePtr HashValue::fetch_values(Env *env, size_t argc, ValuePtr *args, Block *block) {
    auto array = new ArrayValue {};
    if (argc == 0) return array;

    for (size_t i = 0; i < argc; ++i) {
        array->push(fetch(env, args[i], nullptr, block));
    }
    return array;
}

ValuePtr HashValue::keys(Env *env) {
    ArrayValue *array = new ArrayValue {};
    for (HashValue::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

ValuePtr HashValue::keep_if(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("keep_if") });

    assert_not_frozen(env);
    for (auto &node : *this) {
        ValuePtr args[2] = { node.key, node.val };
        if (!NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr)->is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

ValuePtr HashValue::to_h(Env *env, Block *block) {
    if (!block)
        return this;

    auto copy = new HashValue {};
    ValuePtr block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (auto &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr);
        if (!result->is_array() && result->respond_to(env, SymbolValue::intern("to_ary")))
            result = result.send(env, SymbolValue::intern("to_ary"));
        if (!result->is_array())
            env->raise("TypeError", "wrong element type {} (expected array)", result->klass()->class_name_or_blank());
        auto result_array = result->as_array();
        if (result_array->size() != 2)
            env->raise("ArgumentError", "element has wrong array length (expected 2, was {})", result_array->size());
        copy->put(env, (*result_array)[0], (*result_array)[1]);
    }

    return copy;
}

ValuePtr HashValue::values(Env *env) {
    ArrayValue *array = new ArrayValue {};
    for (HashValue::Key &node : *this) {
        array->push(node.val);
    }
    return array;
}

ValuePtr HashValue::has_key(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return TrueValue::the();
    } else {
        return FalseValue::the();
    }
}

ValuePtr HashValue::has_value(Env *env, ValuePtr value) {
    for (auto &node : *this) {
        if (node.val.send(env, SymbolValue::intern("=="), { value })->is_true()) {
            return TrueValue::the();
        }
    }
    return FalseValue::the();
}

ValuePtr HashValue::merge(Env *env, size_t argc, ValuePtr *args, Block *block) {
    return dup(env)->as_hash()->merge_in_place(env, argc, args, block);
}

ValuePtr HashValue::merge_in_place(Env *env, size_t argc, ValuePtr *args, Block *block) {
    this->assert_not_frozen(env);

    for (size_t i = 0; i < argc; i++) {
        auto h = args[i];

        if (!h->is_hash() && h->respond_to_method(env, SymbolValue::intern("to_hash")))
            h = h->send(env, SymbolValue::intern("to_hash"));

        h->assert_type(env, Value::Type::Hash, "Hash");

        for (auto node : *h->as_hash()) {
            auto new_value = node.val;
            if (block) {
                auto old_value = get(env, node.key);
                if (old_value) {
                    ValuePtr args[3] = { node.key, old_value, new_value };
                    new_value = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 3, args, nullptr);
                }
            }
            put(env, node.key, new_value);
        }
    }
    return this;
}

ValuePtr HashValue::slice(Env *env, size_t argc, ValuePtr *args) {
    auto new_hash = new HashValue {};
    for (size_t i = 0; i < argc; i++) {
        ValuePtr key = args[i];
        ValuePtr value = this->get(env, key);
        if (value) {
            new_hash->put(env, key, value);
        }
    }
    return new_hash;
}

void HashValue::visit_children(Visitor &visitor) {
    Value::visit_children(visitor);
    for (auto pair : m_hashmap) {
        visitor.visit(pair.first);
        visitor.visit(pair.first->key);
        visitor.visit(pair.first->val);
        visitor.visit(pair.second);
    }
    visitor.visit(m_default_value);
    visitor.visit(m_default_proc);
}

ValuePtr HashValue::compact(Env *env) {
    auto new_hash = new HashValue {};
    auto nil = NilValue::the();
    for (auto pair : m_hashmap) {
        if (pair.second != nil)
            new_hash->put(env, pair.first->key, pair.second);
    }
    return new_hash;
}

ValuePtr HashValue::compact_in_place(Env *env) {
    assert_not_frozen(env);
    auto nil = NilValue::the();
    auto to_remove = TM::Vector<ValuePtr> {};
    for (auto pair : m_hashmap) {
        if (pair.second == nil) {
            to_remove.push(pair.first->key);
        }
    }
    for (auto key : to_remove)
        remove(env, key);
    if (to_remove.is_empty())
        return NilValue::the();
    return this;
}
}
