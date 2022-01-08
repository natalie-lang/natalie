#include "natalie.hpp"
#include "tm/recursion_guard.hpp"
#include "tm/vector.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
size_t HashObject::hash(const void *key) {
    return static_cast<const HashObject::Key *>(key)->hash;
}

// this is used by the hashmap library to compare keys
bool HashObject::compare(const void *a, const void *b, void *env) {
    assert(env);
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;

    if (a_p->key->object_id() == b_p->key->object_id() && a_p->hash == b_p->hash)
        return true;

    return a_p->key.send((Env *)env, "eql?"_s, { b_p->key })->is_truthy();
}

Value HashObject::compare_by_identity(Env *env) {
    assert_not_frozen(env);
    this->m_is_comparing_by_identity = true;
    this->rehash(env);
    return this;
}

bool HashObject::is_comparing_by_identity() const {
    if (m_is_comparing_by_identity) {
        return true;
    } else {
        return false;
    }
}

Value HashObject::get(Env *env, Value key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = generate_key_hash(env, key);
    return m_hashmap.get(&key_container, env);
}

nat_int_t HashObject::generate_key_hash(Env *env, Value key) const {
    if (m_is_comparing_by_identity) {
        return TM::Hashmap<void *>::hash_ptr(key.object());
    } else {
        return key.send(env, "hash"_s)->as_integer()->to_nat_int_t();
    }
}

Value HashObject::get_default(Env *env, Value key) {
    if (m_default_proc) {
        if (!key)
            return NilObject::the();
        Value args[2] = { this, key };
        return m_default_proc->call(env, 2, args, nullptr);
    } else {
        return m_default_value;
    }
}

Value HashObject::set_default(Env *env, Value value) {
    assert_not_frozen(env);
    m_default_value = value;
    m_default_proc = nullptr;
    return value;
}

void HashObject::put(Env *env, Value key, Value val) {
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
        entry->value = val;
    } else {
        if (m_is_iterating) {
            env->raise("RuntimeError", "can't add a new key into hash during iteration");
        }
        auto *key_container = key_list_append(env, key, hash, val);
        m_hashmap.put(key_container, val, env);
    }
}

Value HashObject::remove(Env *env, Value key) {
    Key key_container;
    key_container.key = key;
    auto hash = generate_key_hash(env, key);
    key_container.hash = hash;
    auto entry = m_hashmap.find_item(&key_container, hash, env);
    if (entry) {
        key_list_remove_node((Key *)entry->key);
        auto val = entry->value;
        m_hashmap.remove(&key_container, env);
        return val;
    } else {
        return nullptr;
    }
}

Value HashObject::clear(Env *env) {
    assert_not_frozen(env);
    m_hashmap.clear();
    m_key_list = nullptr;
    m_is_iterating = false;
    return this;
}

Value HashObject::default_proc(Env *env) {
    return m_default_proc;
}

Value HashObject::set_default_proc(Env *env, Value value) {
    assert_not_frozen(env);
    if (value == NilObject::the()) {
        m_default_proc = nullptr;
        return value;
    }
    auto to_proc = "to_proc"_s;
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

HashObject::Key *HashObject::key_list_append(Env *env, Value key, nat_int_t hash, Value val) {
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

void HashObject::key_list_remove_node(Key *node) {
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

Value HashObject::initialize(Env *env, Value default_value, Block *block) {
    assert_not_frozen(env);

    if (block) {
        if (default_value) {
            env->raise("ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_proc(new ProcObject { block });
    } else {
        if (!default_value) {
            default_value = NilObject::the();
        }

        set_default(env, default_value);
    }
    return this;
}

// Hash[]
Value HashObject::square_new(Env *env, size_t argc, Value *args, ClassObject *klass) {
    if (argc == 0) {
        return new HashObject { klass };
    } else if (argc == 1) {
        Value value = args[0];
        if (!value->is_hash() && value->respond_to(env, "to_hash"_s))
            value = value.send(env, "to_hash"_s);
        if (value->is_hash()) {
            auto hash = new HashObject { env, *value->as_hash() };
            hash->m_klass = klass;
            return hash;
        } else {
            if (!value->is_array() && value->respond_to(env, "to_ary"_s))
                value = value.send(env, "to_ary"_s);
            if (value->is_array()) {
                HashObject *hash = new HashObject { klass };
                for (auto &pair : *value->as_array()) {
                    if (pair->type() != Object::Type::Array) {
                        env->raise("ArgumentError", "wrong element in array to Hash[]");
                    }
                    size_t size = pair->as_array()->size();
                    if (size < 1 || size > 2) {
                        env->raise("ArgumentError", "invalid number of elements ({} for 1..2)", size);
                    }
                    Value key = (*pair->as_array())[0];
                    Value value = size == 1 ? NilObject::the() : (*pair->as_array())[1];
                    hash->put(env, key, value);
                }
                return hash;
            }
        }
    }
    if (argc % 2 != 0) {
        env->raise("ArgumentError", "odd number of arguments for Hash");
    }
    HashObject *hash = new HashObject { klass };
    for (size_t i = 0; i < argc; i += 2) {
        Value key = args[i];
        Value value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

Value HashObject::inspect(Env *env) {
    TM::RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return new StringObject("{...}");
        StringObject *out = new StringObject { "{" };
        size_t last_index = size() - 1;
        size_t index = 0;

        auto to_s = [env](Value obj) {
            if (obj->is_string())
                return obj->as_string();
            if (obj->respond_to(env, "to_s"_s))
                obj = obj->send(env, "to_s"_s);
            else
                obj = new StringObject("?");
            if (!obj->is_string())
                obj = StringObject::format(env, "#<{}:{}>", obj->klass()->inspect_str(), int_to_hex_string(obj->object_id(), false));
            return obj->as_string();
        };

        for (HashObject::Key &node : *this) {
            StringObject *key_repr = to_s(node.key.send(env, "inspect"_s));
            out->append(env, key_repr);
            out->append(env, "=>");
            StringObject *val_repr = to_s(node.val.send(env, "inspect"_s));
            out->append(env, val_repr);
            if (index < last_index) {
                out->append(env, ", ");
            }
            index++;
        }

        out->append_char('}');
        return out;
    });
}

Value HashObject::ref(Env *env, Value key) {
    Value val = get(env, key);
    if (val) {
        return val;
    } else {
        return send(env, "default"_s, { key });
    }
}

Value HashObject::refeq(Env *env, Value key, Value val) {
    put(env, key, val);
    return val;
}

Value HashObject::rehash(Env *env) {
    assert_not_frozen(env);

    auto copy = new HashObject { env, *this };
    HashObject::operator=(std::move(*copy));

    return this;
}

Value HashObject::replace(Env *env, Value other) {
    if (!other->is_hash() && other->respond_to(env, "to_hash"_s))
        other = other->send(env, "to_hash"_s);
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

Value HashObject::delete_if(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "delete_if"_s });

    assert_not_frozen(env);
    for (auto &node : *this) {
        Value args[2] = { node.key, node.val };
        if (NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr)->is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

Value HashObject::delete_key(Env *env, Value key, Block *block) {
    assert_not_frozen(env);
    Value val = remove(env, key);
    if (val)
        return val;
    else if (block)
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 0, nullptr, nullptr);
    else
        return NilObject::the();
}

Value HashObject::dig(Env *env, size_t argc, Value *args) {
    env->ensure_argc_at_least(argc, 1);
    auto dig = "dig"_s;
    Value val = ref(env, args[0]);
    if (argc == 1)
        return val;

    if (val == NilObject::the())
        return val;

    if (!val->respond_to(env, dig))
        env->raise("TypeError", "{} does not have #dig method", val->klass()->inspect_str());

    return val.send(env, dig, argc - 1, args + 1);
}

Value HashObject::size(Env *env) const {
    return IntegerObject::from_size_t(env, size());
}

bool HashObject::eq(Env *env, Value other_value, SymbolObject *method_name) {
    TM::PairedRecursionGuard guard { this, other_value.object() };

    return guard.run([&](bool is_recursive) -> bool {
        if (!other_value->is_hash())
            return false;

        HashObject *other = other_value->as_hash();
        if (size() != other->size())
            return false;

        if (is_recursive)
            return true;

        Value other_val;
        for (HashObject::Key &node : *this) {
            other_val = other->get(env, node.key);
            if (!other_val)
                return false;

            if (node.val == other_val)
                continue;

            if (node.val.send(env, method_name, { other_val })->is_falsey())
                return false;
        }

        return true;
    });
}

bool HashObject::eq(Env *env, Value other_value) {
    return eq(env, other_value, "=="_s);
}

bool HashObject::eql(Env *env, Value other_value) {
    return eq(env, other_value, "eql?"_s);
}

bool HashObject::gte(Env *env, Value other) {
    if (!other->is_hash() && other->respond_to(env, "to_hash"_s))
        other = other->send(env, "to_hash"_s);

    other->assert_type(env, Object::Type::Hash, "Hash");
    auto other_hash = other->as_hash();

    for (auto &node : *other_hash) {
        Value value = get(env, node.key);
        if (!value || value.send(env, "=="_s, { node.val })->is_false()) {
            return false;
        }
    }
    return true;
}

bool HashObject::gt(Env *env, Value other) {
    if (!other->is_hash() && other->respond_to(env, "to_hash"_s))
        other = other->send(env, "to_hash"_s);

    other->assert_type(env, Object::Type::Hash, "Hash");

    return gte(env, other) && other->as_hash()->size() != size();
}

bool HashObject::lte(Env *env, Value other) {
    if (!other->is_hash() && other->respond_to(env, "to_hash"_s))
        other = other->send(env, "to_hash"_s);

    other->assert_type(env, Object::Type::Hash, "Hash");
    auto other_hash = other->as_hash();

    for (auto &node : *this) {
        Value value = other_hash->get(env, node.key);
        if (!value || value.send(env, "=="_s, { node.val })->is_false()) {
            return false;
        }
    }
    return true;
}

bool HashObject::lt(Env *env, Value other) {
    if (!other->is_hash() && other->respond_to(env, "to_hash"_s))
        other = other->send(env, "to_hash"_s);

    other->assert_type(env, Object::Type::Hash, "Hash");

    return lte(env, other) && other->as_hash()->size() != size();
}

Value HashObject::each(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "each"_s });

    Value block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (HashObject::Key &node : *this) {
        auto ary = new ArrayObject { { node.key, node.val } };
        Value block_args[1] = { ary };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, block_args, nullptr);
    }
    return this;
}

Value HashObject::except(Env *env, size_t argc, Value *args) {
    HashObject *new_hash = new HashObject {};
    for (auto &node : *this) {
        new_hash->put(env, node.key, node.val);
    }

    for (size_t i = 0; i < argc; i++) {
        new_hash->remove(env, args[i]);
    }
    return new_hash;
}

Value HashObject::fetch(Env *env, Value key, Value default_value, Block *block) {
    Value value = get(env, key);
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

Value HashObject::fetch_values(Env *env, size_t argc, Value *args, Block *block) {
    if (argc == 0) return new ArrayObject;

    auto array = new ArrayObject { argc };
    for (size_t i = 0; i < argc; ++i) {
        array->push(fetch(env, args[i], nullptr, block));
    }
    return array;
}

Value HashObject::keys(Env *env) {
    ArrayObject *array = new ArrayObject { size() };
    for (HashObject::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

Value HashObject::keep_if(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "keep_if"_s });

    assert_not_frozen(env);
    for (auto &node : *this) {
        Value args[2] = { node.key, node.val };
        if (!NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr)->is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

Value HashObject::to_h(Env *env, Block *block) {
    if (!block)
        return this;

    auto copy = new HashObject {};
    Value block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (auto &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr);
        if (!result->is_array() && result->respond_to(env, "to_ary"_s))
            result = result.send(env, "to_ary"_s);
        if (!result->is_array())
            env->raise("TypeError", "wrong element type {} (expected array)", result->klass()->inspect_str());
        auto result_array = result->as_array();
        if (result_array->size() != 2)
            env->raise("ArgumentError", "element has wrong array length (expected 2, was {})", result_array->size());
        copy->put(env, (*result_array)[0], (*result_array)[1]);
    }

    return copy;
}

Value HashObject::values(Env *env) {
    ArrayObject *array = new ArrayObject { size() };
    for (HashObject::Key &node : *this) {
        array->push(node.val);
    }
    return array;
}

Value HashObject::hash(Env *env) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return Value { NilObject::the() };

        HashBuilder hash { 10889, false };
        auto hash_method = "hash"_s;
        auto to_int = "to_int"_s;

        for (HashObject::Key &node : *this) {
            HashBuilder entry_hash {};
            bool any_change = false;

            auto value = node.val;
            if (!eql(env, value)) {
                auto value_hash = value->send(env, hash_method);

                if (!value_hash->is_nil()) {
                    if (!value_hash->is_integer() && value_hash->respond_to(env, to_int))
                        value_hash = value_hash->send(env, to_int);

                    entry_hash.append(value_hash->as_integer()->to_nat_int_t());
                    any_change = true;
                }
            }

            auto key = node.key;
            if (!eql(env, key)) {
                auto key_hash = key->send(env, hash_method);

                if (!key_hash->is_nil()) {
                    if (!key_hash->is_integer() && key_hash->respond_to(env, to_int))
                        key_hash = key_hash->send(env, to_int);

                    entry_hash.append(key_hash->as_integer()->to_nat_int_t());
                    any_change = true;
                }
            }

            if (any_change)
                hash.append(entry_hash.digest());
        }

        return Value::integer(hash.digest());
    });
}

bool HashObject::has_key(Env *env, Value key) {
    Value val = get(env, key);
    if (val) {
        return true;
    } else {
        return false;
    }
}

bool HashObject::has_value(Env *env, Value value) {
    for (auto &node : *this) {
        if (node.val.send(env, "=="_s, { value })->is_true()) {
            return true;
        }
    }
    return false;
}

Value HashObject::merge(Env *env, size_t argc, Value *args, Block *block) {
    return dup(env)->as_hash()->merge_in_place(env, argc, args, block);
}

Value HashObject::merge_in_place(Env *env, size_t argc, Value *args, Block *block) {
    this->assert_not_frozen(env);

    for (size_t i = 0; i < argc; i++) {
        auto h = args[i];

        if (!h->is_hash() && h->respond_to(env, "to_hash"_s))
            h = h->send(env, "to_hash"_s);

        h->assert_type(env, Object::Type::Hash, "Hash");

        for (auto node : *h->as_hash()) {
            auto new_value = node.val;
            if (block) {
                auto old_value = get(env, node.key);
                if (old_value) {
                    Value args[3] = { node.key, old_value, new_value };
                    new_value = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 3, args, nullptr);
                }
            }
            put(env, node.key, new_value);
        }
    }
    return this;
}

Value HashObject::slice(Env *env, size_t argc, Value *args) {
    auto new_hash = new HashObject {};
    for (size_t i = 0; i < argc; i++) {
        Value key = args[i];
        Value value = this->get(env, key);
        if (value) {
            new_hash->put(env, key, value);
        }
    }
    return new_hash;
}

void HashObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto pair : m_hashmap) {
        visitor.visit(pair.first);
        visitor.visit(pair.first->key);
        visitor.visit(pair.first->val);
        visitor.visit(pair.second);
    }
    visitor.visit(m_default_value);
    visitor.visit(m_default_proc);
}

Value HashObject::compact(Env *env) {
    auto new_hash = new HashObject {};
    auto nil = NilObject::the();
    for (auto pair : m_hashmap) {
        if (pair.second != nil)
            new_hash->put(env, pair.first->key, pair.second);
    }
    return new_hash;
}

Value HashObject::compact_in_place(Env *env) {
    assert_not_frozen(env);
    auto nil = NilObject::the();
    auto to_remove = TM::Vector<Value> {};
    for (auto pair : m_hashmap) {
        if (pair.second == nil) {
            to_remove.push(pair.first->key);
        }
    }
    for (auto key : to_remove)
        remove(env, key);
    if (to_remove.is_empty())
        return NilObject::the();
    return this;
}
}
