#include "natalie.hpp"
#include "natalie/integer_methods.hpp"
#include "tm/recursion_guard.hpp"
#include "tm/vector.hpp"

namespace TM {

size_t HashKeyHandler<Natalie::HashKey *>::hash(Natalie::HashKey *key) {
    return key->hash;
}

// this is used by the hashmap library to compare keys
bool HashKeyHandler<Natalie::HashKey *>::compare(Natalie::HashKey *a, Natalie::HashKey *b, void *env) {
    assert(env);

    if (Natalie::Object::object_id(a->key) == Natalie::Object::object_id(b->key) && a->hash == b->hash)
        return true;

    return a->key.send((Natalie::Env *)env, Natalie::SymbolObject::intern("eql?"), { b->key }).is_truthy();
}

}

namespace Natalie {

bool HashObject::is_ruby2_keywords_hash(Env *env, Value hash) {
    return hash.as_hash_or_raise(env)->m_is_ruby2_keywords_hash;
}

Value HashObject::ruby2_keywords_hash(Env *env, Value hash) {
    auto result = hash.as_hash_or_raise(env)->duplicate(env).as_hash();
    result->m_is_ruby2_keywords_hash = true;
    return result;
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

Optional<Value> HashObject::get(Env *env, Value key) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    HashKey key_container;
    key_container.key = key;
    key_container.hash = generate_key_hash(env, key);
    return m_hashmap.get(&key_container, env);
}

nat_int_t HashObject::generate_key_hash(Env *env, Value key) const {
    if (m_is_comparing_by_identity && !key.is_float() && !key.is_integer()) {
        auto obj = key.object();
        return TM::HashKeyHandler<void *>::hash(obj);
    } else {
        return key.send(env, "hash"_s).integer().to_nat_int_t();
    }
}

Value HashObject::get_default(Env *env, Optional<Value> key) {
    if (m_default_proc) {
        if (!key)
            return Value::nil();
        return m_default_proc->call(env, { this, key.value() }, nullptr);
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
    NAT_GC_GUARD_VALUE(key);
    NAT_GC_GUARD_VALUE(val);
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    assert_not_frozen(env);
    HashKey key_container;
    if (!m_is_comparing_by_identity && key.is_string() && !key->is_frozen()) {
        key = key.as_string()->duplicate(env);
    }
    key_container.key = key;

    auto hash = generate_key_hash(env, key);
    key_container.hash = hash;
    auto entry = m_hashmap.find_item(&key_container, hash, env);
    if (entry) {
        ((HashKey *)entry->key)->val = val;
        entry->value = val;
    } else {
        if (m_is_iterating) {
            env->raise("RuntimeError", "can't add a new key into hash during iteration");
        }
        auto *key_container = key_list_append(env, key, hash, val);
        m_hashmap.put(key_container, val, env);
    }
}

Optional<Value> HashObject::remove(Env *env, Value key) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    HashKey key_container;
    key_container.key = key;
    auto hash = generate_key_hash(env, key);
    key_container.hash = hash;
    auto entry = m_hashmap.find_item(&key_container, hash, env);
    if (entry) {
        key_list_remove_node((HashKey *)entry->key);
        auto val = entry->value;
        m_hashmap.remove(&key_container, env);
        return val;
    } else {
        return {};
    }
}

Value HashObject::clear(Env *env) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    assert_not_frozen(env);
    m_hashmap.clear();
    m_key_list = nullptr;
    m_is_iterating = false;
    return this;
}

Value HashObject::default_proc(Env *env) {
    if (m_default_proc == nullptr)
        return Value::nil();
    return m_default_proc;
}

Value HashObject::set_default_proc(Env *env, Value value) {
    assert_not_frozen(env);
    if (value == Value::nil()) {
        m_default_proc = nullptr;
        return value;
    }
    auto to_proc = "to_proc"_s;
    auto to_proc_value = value;
    if (value.respond_to(env, to_proc))
        to_proc_value = value.send(env, to_proc);
    to_proc_value.assert_type(env, Type::Proc, "Proc");
    auto proc = to_proc_value.as_proc();
    auto arity = proc->arity();
    if (proc->is_lambda() && arity != 2)
        env->raise("TypeError", "default_proc takes two arguments (2 for {})", (long long)arity);
    m_default_proc = proc;
    return value;
}

Natalie::HashKey *HashObject::key_list_append(Env *env, Value key, nat_int_t hash, Value val) {
    if (m_key_list) {
        HashKey *first = m_key_list;
        HashKey *last = m_key_list->prev;
        HashKey *new_last = HashKey::create(key, val, hash);
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        HashKey *node = HashKey::create(key, val, hash);
        node->prev = node;
        node->next = node;
        node->removed = false;
        m_key_list = node;
        return node;
    }
}

void HashObject::key_list_remove_node(HashKey *node) {
    HashKey *prev = node->prev;
    HashKey *next = node->next;
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

Value HashObject::initialize(Env *env, Optional<Value> default_arg, Optional<Value> capacity, Block *block) {
    assert_not_frozen(env);

    if (capacity) {
        const auto capacity_int = IntegerMethods::convert_to_native_type<ssize_t>(env, capacity.value());
        if (capacity_int > 0)
            m_hashmap = TM::Hashmap<HashKey *, Optional<Value>> { static_cast<size_t>(capacity_int) };
    }

    if (block) {
        if (default_arg) {
            env->raise("ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_proc(new ProcObject { block });
    } else {
        set_default(env, default_arg.value_or(Value::nil()));
    }
    return this;
}

// Hash[]
Value HashObject::square_new(Env *env, ClassObject *klass, Args &&args) {
    if (args.size() == 0) {
        return HashObject::create(klass);
    } else if (args.size() == 1) {
        Value value = args[0];
        if (!value.is_hash() && value.respond_to(env, "to_hash"_s))
            value = value.to_hash(env);
        if (value.is_hash()) {
            auto hash = HashObject::create(env, *value.as_hash());
            hash->m_default_proc = nullptr;
            hash->m_default_value = Value::nil();
            hash->m_klass = klass;
            return hash;
        } else {
            if (!value.is_array() && value.respond_to(env, "to_ary"_s))
                value = value.to_ary(env);
            if (value.is_array()) {
                HashObject *hash = HashObject::create(klass);
                for (auto &pair : *value.as_array()) {
                    if (!pair.is_array()) {
                        env->raise("ArgumentError", "wrong element in array to Hash[]");
                    }
                    size_t size = pair.as_array()->size();
                    if (size < 1 || size > 2) {
                        env->raise("ArgumentError", "invalid number of elements ({} for 1..2)", size);
                    }
                    Value key = (*pair.as_array())[0];
                    Value value = size == 1 ? Value::nil() : (*pair.as_array())[1];
                    hash->put(env, key, value);
                }
                return hash;
            }
        }
    }
    if (args.size() % 2 != 0) {
        env->raise("ArgumentError", "odd number of arguments for Hash");
    }
    HashObject *hash = HashObject::create(klass);
    for (size_t i = 0; i < args.size(); i += 2) {
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
            return StringObject::create("{...}");
        StringObject *out = StringObject::create("{");
        size_t last_index = size() - 1;
        size_t index = 0;

        auto to_s = [env](Value obj) {
            if (obj.is_string())
                return obj.as_string();
            if (obj.respond_to(env, "to_s"_s))
                obj = obj.send(env, "to_s"_s);
            else
                obj = StringObject::create("?");
            if (!obj.is_string())
                obj = StringObject::format("#<{}:{}>", obj.klass()->inspect_module(), String::hex(object_id(obj), String::HexFormat::LowercaseAndPrefixed));
            return obj.as_string();
        };

        for (HashKey &node : *this) {
            if (node.key.is_symbol()) {
                SymbolObject *key = node.key.as_symbol();
                StringObject *key_repr = nullptr;
                if (key->should_be_quoted()) {
                    key_repr = key->inspect(env)->delete_prefix(env, StringObject::create(":")).as_string();
                } else {
                    key_repr = key->to_s(env);
                }
                out->append(key_repr);
                out->append(": ");
            } else {
                StringObject *key_repr = to_s(node.key.send(env, "inspect"_s));
                out->append(key_repr);
                out->append(" => ");
            }
            StringObject *val_repr = to_s(node.val.send(env, "inspect"_s));
            out->append(val_repr);
            if (index < last_index) {
                out->append(", ");
            }
            index++;
        }

        out->append_char('}');
        return out;
    });
}

String HashObject::dbg_inspect(int indent) const {
    auto str = String::format("<HashObject {h} size={} {", this, size());
    size_t index = 0;
    for (auto pair : *this) {
        str.append_char('\n');
        str.append_char(' ', indent + 2);
        str.append(pair.key.dbg_inspect(indent + 2));
        str.append(" => ");
        str.append(pair.val.dbg_inspect(indent + 2));
        if (index < size() - 1)
            str.append_char(',');
        index++;
    }
    str.append("}>");
    return str;
}

Value HashObject::ref(Env *env, Value key) {
    auto val = get(env, key);
    if (!val)
        return send(env, "default"_s, { key });
    return val.value();
}

Value HashObject::refeq(Env *env, Value key, Value val) {
    put(env, key, val);
    return val;
}

Value HashObject::rehash(Env *env) {
    assert_not_frozen(env);

    auto copy = HashObject::create(env, *this);
    HashObject::operator=(std::move(*copy));

    return this;
}

Value HashObject::replace(Env *env, Value other) {
    assert_not_frozen(env);

    auto other_hash = other.to_hash(env);
    if (this == other_hash)
        return this;

    m_is_comparing_by_identity = other_hash->m_is_comparing_by_identity;

    clear(env);
    for (auto node : *other_hash) {
        put(env, node.key, node.val);
    }
    m_default_value = other_hash->m_default_value;
    m_default_proc = other_hash->m_default_proc;
    return this;
}

Value HashObject::delete_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, HashObject::size_fn, 0);
        return send(env, "enum_for"_s, { "delete_if"_s }, size_block);
    }

    assert_not_frozen(env);
    for (auto &node : *this) {
        Value args[2] = { node.key, node.val };
        if (block->run(env, Args(2, args), nullptr).is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

Value HashObject::delete_key(Env *env, Value key, Block *block) {
    assert_not_frozen(env);
    auto val = remove(env, key);
    if (val)
        return val.value();
    else if (block)
        return block->run(env, {}, nullptr);
    else
        return Value::nil();
}

Value HashObject::dig(Env *env, Args &&args) {
    args.ensure_argc_at_least(env, 1);
    auto dig = "dig"_s;
    auto idx = args.shift();
    Value val = ref(env, idx);
    if (args.size() == 0)
        return val;

    if (val == Value::nil())
        return val;

    if (!val.respond_to(env, dig))
        env->raise("TypeError", "{} does not have #dig method", val.klass()->inspect_module());

    return val.send(env, dig, std::move(args));
}

Value HashObject::size(Env *env) const {
    return IntegerMethods::from_size_t(env, size());
}

bool HashObject::eq(Env *env, Value other_value, SymbolObject *method_name) {
    auto lambda = [&](bool is_recursive) -> bool {
        if (!other_value.is_hash())
            return false;

        HashObject *other = other_value.as_hash();
        if (size() != other->size())
            return false;

        if (is_recursive)
            return true;

        for (HashKey &node : *this) {
            auto other_optional_val = other->get(env, node.key);
            if (!other_optional_val)
                return false;

            auto other_val = other_optional_val.value();

            if (node.val == other_val)
                continue;

            if (node.val.send(env, method_name, { other_val }).is_falsey())
                return false;
        }

        return true;
    };

    if (!other_value.has_heap_object())
        return lambda(false);

    TM::PairedRecursionGuard guard { this, other_value.object() };
    return guard.run(lambda);
}

bool HashObject::eq(Env *env, Value other_value) {
    return eq(env, other_value, "=="_s);
}

bool HashObject::eql(Env *env, Value other_value) {
    return eq(env, other_value, "eql?"_s);
}

bool HashObject::gte(Env *env, Value other) {
    auto other_hash = other.to_hash(env);

    for (auto &node : *other_hash) {
        auto value = get(env, node.key);
        if (!value || value.value().send(env, "=="_s, { node.val }).is_false()) {
            return false;
        }
    }
    return true;
}

bool HashObject::gt(Env *env, Value other) {
    auto other_hash = other.to_hash(env);

    return gte(env, other) && other_hash->size() != size();
}

bool HashObject::lte(Env *env, Value other) {
    auto other_hash = other.to_hash(env);

    for (auto &node : *this) {
        auto value = other_hash->get(env, node.key);
        if (!value || value.value().send(env, "=="_s, { node.val }).is_false()) {
            return false;
        }
    }
    return true;
}

bool HashObject::lt(Env *env, Value other) {
    auto other_hash = other.to_hash(env);

    return lte(env, other) && other_hash->size() != size();
}

Value HashObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, HashObject::size_fn, 0);
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }

    Value block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (HashKey &node : *this) {
        auto ary = ArrayObject::create({ node.key, node.val });
        Value block_args[1] = { ary };
        block->run(env, Args(1, block_args), nullptr);
    }
    return this;
}

Value HashObject::except(Env *env, Args &&args) {
    HashObject *new_hash = HashObject::create();
    for (auto &node : *this) {
        new_hash->put(env, node.key, node.val);
    }

    for (size_t i = 0; i < args.size(); i++) {
        new_hash->remove(env, args[i]);
    }
    return new_hash;
}

Value HashObject::fetch(Env *env, Value key, Optional<Value> default_value, Block *block) {
    auto value = get(env, key);
    if (!value) {
        if (block) {
            if (default_value)
                env->warn("block supersedes default value argument");

            Value args[] = { key };
            value = block->run(env, Args(1, args), nullptr);
        } else if (default_value) {
            value = default_value.value();
        } else {
            env->raise_key_error(this, key);
        }
    }
    return value.value();
}

Value HashObject::fetch_values(Env *env, Args &&args, Block *block) {
    if (args.size() == 0) return ArrayObject::create();

    auto array = ArrayObject::create(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        array->push(fetch(env, args[i], {}, block));
    }
    return array;
}

Value HashObject::keys(Env *env) {
    ArrayObject *array = ArrayObject::create(size());
    for (HashKey &node : *this) {
        array->push(node.key);
    }
    return array;
}

Value HashObject::keep_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, HashObject::size_fn, 0);
        return send(env, "enum_for"_s, { "keep_if"_s }, size_block);
    }

    assert_not_frozen(env);
    for (auto &node : *this) {
        Value args[2] = { node.key, node.val };
        if (!block->run(env, Args(2, args), nullptr).is_truthy()) {
            delete_key(env, node.key, nullptr);
        }
    }

    return this;
}

Value HashObject::to_h(Env *env, Block *block) {
    if (!block) {
        if (m_klass == GlobalEnv::the()->Hash()) {
            return this;
        } else {
            auto res = duplicate(env).as_hash();
            res->m_klass = GlobalEnv::the()->Hash();
            return res;
        }
    }

    auto copy = HashObject::create();
    Value block_args[2];
    set_is_iterating(true);
    Defer no_longer_iterating([&]() { set_is_iterating(false); });
    for (auto &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        auto result = block->run(env, Args(2, block_args), nullptr);
        if (!result.is_array() && result.respond_to(env, "to_ary"_s))
            result = result.to_ary(env);
        if (!result.is_array())
            env->raise("TypeError", "wrong element type {} (expected array)", result.klass()->inspect_module());
        auto result_array = result.as_array();
        if (result_array->size() != 2)
            env->raise("ArgumentError", "element has wrong array length (expected 2, was {})", result_array->size());
        copy->put(env, (*result_array)[0], (*result_array)[1]);
    }

    return copy;
}

Value HashObject::values(Env *env) {
    ArrayObject *array = ArrayObject::create(size());
    for (HashKey &node : *this) {
        array->push(node.val);
    }
    return array;
}

Value HashObject::hash(Env *env) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return Value { Value::nil() };

        HashBuilder hash { 10889, false };
        auto hash_method = "hash"_s;

        for (HashKey &node : *this) {
            HashBuilder entry_hash {};
            bool any_change = false;

            auto value = node.val;
            if (!eql(env, value)) {
                auto value_hash = value.send(env, hash_method);

                if (!value_hash.is_nil()) {
                    entry_hash.append(IntegerMethods::convert_to_nat_int_t(env, value_hash));
                    any_change = true;
                }
            }

            auto key = node.key;
            if (!eql(env, key)) {
                auto key_hash = key.send(env, hash_method);

                if (!key_hash.is_nil()) {
                    entry_hash.append(IntegerMethods::convert_to_nat_int_t(env, key_hash));
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
    return get(env, key).present();
}

bool HashObject::has_value(Env *env, Value value) {
    for (auto &node : *this) {
        if (node.val.send(env, "=="_s, { value }).is_true()) {
            return true;
        }
    }
    return false;
}

Value HashObject::merge(Env *env, Args &&args, Block *block) {
    return duplicate(env).as_hash()->merge_in_place(env, std::move(args), block);
}

Value HashObject::merge_in_place(Env *env, Args &&args, Block *block) {
    this->assert_not_frozen(env);

    for (size_t i = 0; i < args.size(); i++) {
        for (auto node : *args[i].to_hash(env)) {
            auto new_value = node.val;
            if (block) {
                auto old_value = get(env, node.key);
                if (old_value) {
                    Value args[3] = { node.key, old_value.value(), new_value };
                    new_value = block->run(env, Args(3, args), nullptr);
                }
            }
            put(env, node.key, new_value);
        }
    }
    return this;
}

Value HashObject::slice(Env *env, Args &&args) {
    auto new_hash = HashObject::create();
    for (size_t i = 0; i < args.size(); i++) {
        Value key = args[i];
        auto value = this->get(env, key);
        if (value)
            new_hash->put(env, key, value.value());
    }
    return new_hash;
}

void HashObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    for (auto pair : m_hashmap) {
        visitor.visit(pair.first);
        visitor.visit(pair.first->key);
        visitor.visit(pair.first->val);
        if (pair.second)
            visitor.visit(pair.second.value());
    }
    visitor.visit(m_default_value);
    visitor.visit(m_default_proc);
}

Value HashObject::compact(Env *env) {
    auto new_hash = HashObject::create();
    new_hash->m_default_value = m_default_value;
    new_hash->m_default_proc = m_default_proc;
    new_hash->m_is_comparing_by_identity = m_is_comparing_by_identity;
    for (auto pair : m_hashmap) {
        auto val = pair.second.value();
        if (!val.is_nil())
            new_hash->put(env, pair.first->key, val);
    }
    return new_hash;
}

Value HashObject::compact_in_place(Env *env) {
    assert_not_frozen(env);
    auto to_remove = TM::Vector<Value> {};
    for (auto pair : m_hashmap) {
        auto val = pair.second.value();
        if (val.is_nil())
            to_remove.push(pair.first->key);
    }
    for (auto key : to_remove)
        remove(env, key);
    if (to_remove.is_empty())
        return Value::nil();
    return this;
}
}
