#include "natalie.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
nat_int_t HashValue::hash(const void *key) {
    return static_cast<const HashValue::Key *>(key)->hash;
}

// this is used by the hashmap library to compare keys
int HashValue::compare(const void *a, const void *b, Env *env) {
    assert(env);
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;

    if (a_p->key->object_id() == b_p->key->object_id() && a_p->hash == b_p->hash)
        return 0;

    return a_p->key.send(env, "eql?", 1, &b_p->key)->is_truthy() ? 0 : 1; // return 0 for exact match
}

ValuePtr HashValue::get(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    return m_hashmap.get(&key_container, env);
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
    if (key->is_string() && !key->is_frozen()) {
        key = key->as_string()->dup(env);
    }
    key_container.key = key;

    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    auto entry = m_hashmap.find_entry(&key_container, env);
    if (entry) {
        ((Key *)entry->key)->val = val;
        entry->data = val.value();
    } else {
        if (m_is_iterating) {
            env->raise("RuntimeError", "can't add a new key into hash during iteration");
        }
        auto *key_container = key_list_append(env, key, val);
        m_hashmap.put(key_container, val.value(), env);
    }
}

ValuePtr HashValue::remove(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    auto entry = m_hashmap.find_entry(&key_container, env);
    if (entry) {
        key_list_remove_node((Key *)entry->key);
        auto val = (Value *)entry->data;
        m_hashmap.remove(&key_container, env);
        return val;
    } else {
        return nullptr;
    }
}

ValuePtr HashValue::clear(Env *env) {
    assert_not_frozen(env);
    Hashmap<Key *, Value *> blank_hashmap { hash, compare, 256 };
    m_hashmap = std::move(blank_hashmap);
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

HashValue::Key *HashValue::key_list_append(Env *env, ValuePtr key, ValuePtr val) {
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
        new_last->hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
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
        node->hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
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
    if (block) {
        if (default_value) {
            env->raise("ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_proc(new ProcValue { block });
    } else if (default_value) {
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
    StringValue *out = new StringValue { "{" };
    size_t last_index = size() - 1;
    size_t index = 0;
    for (HashValue::Key &node : *this) {
        StringValue *key_repr = node.key.send(env, "inspect")->as_string();
        out->append(env, key_repr);
        out->append(env, "=>");
        StringValue *val_repr = node.val.send(env, "inspect")->as_string();
        out->append(env, val_repr);
        if (index < last_index) {
            out->append(env, ", ");
        }
        index++;
    }
    out->append_char(env, '}');
    return out;
}

ValuePtr HashValue::ref(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return val;
    } else {
        return get_default(env, key);
    }
}

ValuePtr HashValue::refeq(Env *env, ValuePtr key, ValuePtr val) {
    put(env, key, val);
    return val;
}

ValuePtr HashValue::delete_key(Env *env, ValuePtr key) {
    assert_not_frozen(env);
    ValuePtr val = remove(env, key);
    if (val) {
        return val;
    } else {
        return NilValue::the();
    }
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

    return val.send(env, dig, argc - 1, &args[1]);
}

ValuePtr HashValue::size(Env *env) {
    return IntegerValue::from_size_t(env, size());
}

ValuePtr HashValue::eq(Env *env, ValuePtr other_value) {
    if (!other_value->is_hash()) {
        return FalseValue::the();
    }
    HashValue *other = other_value->as_hash();
    if (size() != other->size()) {
        return FalseValue::the();
    }
    ValuePtr other_val;
    for (HashValue::Key &node : *this) {
        other_val = other->get(env, node.key);
        if (!other_val) {
            return FalseValue::the();
        }
        if (!node.val.send(env, "==", 1, &other_val, nullptr)->is_truthy()) {
            return FalseValue::the();
        }
    }
    return TrueValue::the();
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    ValuePtr _result = the_block->_run(env, argc, args, block);                                           \
    if (_result->has_break_flag()) {                                                                      \
        _result->remove_break_flag();                                                                     \
        hash->set_is_iterating(false);                                                                    \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

ValuePtr HashValue::each(Env *env, Block *block) {
    if (!block)
        return _send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each") });

    ValuePtr block_args[2];
    for (HashValue::Key &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, this);
    }
    return this;
}

ValuePtr HashValue::keys(Env *env) {
    ArrayValue *array = new ArrayValue {};
    for (HashValue::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

ValuePtr HashValue::to_h(Env *env, Block *block) {
    if (!block)
        return this;

    auto copy = new HashValue {};
    ValuePtr block_args[2];
    for (auto &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, this);
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

ValuePtr HashValue::sort(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    for (HashValue::Key &node : *this) {
        ArrayValue *pair = new ArrayValue {};
        pair->push(node.key);
        pair->push(node.val);
        ary->push(pair);
    }
    return ary->sort(env, nullptr);
}

ValuePtr HashValue::has_key(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return TrueValue::the();
    } else {
        return FalseValue::the();
    }
}

ValuePtr HashValue::merge(Env *env, size_t argc, ValuePtr *args) {
    return dup(env)->as_hash()->merge_bang(env, argc, args);
}

ValuePtr HashValue::merge_bang(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        auto h = args[i];
        h->assert_type(env, Value::Type::Hash, "Hash");
        for (auto node : *h->as_hash()) {
            put(env, node.key, node.val);
        }
    }
    return this;
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
    bool changed = false;
    for (auto pair : m_hashmap) {
        if (pair.second == nil) {
            remove(env, pair.first->key);
            changed = true;
        }
    }
    if (!changed)
        return NilValue::the();
    return this;
}

}
