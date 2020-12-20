#include "natalie.hpp"
#include <random>

namespace Natalie {

Value *ArrayValue::initialize(Env *env, Value *size, Value *value) {
    if (!size) {
        return this;
    }
    if (size->is_array()) {
        for (auto &item : *size->as_array()) {
            push(item);
        }
        return this;
    }
    size->assert_type(env, Value::Type::Integer, "Integer");
    if (!value) value = env->nil_obj();
    for (nat_int_t i = 0; i < size->as_integer()->to_nat_int_t(); i++) {
        push(value);
    }
    return this;
}

Value *ArrayValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "[" };
    for (size_t i = 0; i < size(); i++) {
        Value *obj = (*this)[i];
        StringValue *repr = obj->send(env, "inspect")->as_string();
        out->append_string(env, repr);
        if (i < size() - 1) {
            out->append(env, ", ");
        }
    }
    out->append_char(env, ']');
    return out;
}

Value *ArrayValue::ltlt(Env *env, Value *arg) {
    this->assert_not_frozen(env);
    push(arg);
    return this;
}

Value *ArrayValue::add(Env *env, Value *other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { *this };
    new_array->concat(*other->as_array());
    return new_array;
}

Value *ArrayValue::sub(Env *env, Value *other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other->as_array()) {
            if (item->send(env, "==", 1, &compare_item, nullptr)->is_truthy()) {
                found = 1;
                break;
            }
        }
        if (!found) {
            new_array->push(item);
        }
    }
    return new_array;
}

Value *ArrayValue::ref(Env *env, Value *index_obj, Value *size) {
    if (index_obj->type() == Value::Type::Integer) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();
        if (index < 0) {
            index = this->size() + index;
        }
        if (index < 0) {
            return env->nil_obj();
        }
        size_t sindex = static_cast<size_t>(index);
        if (sindex >= this->size()) {
            return env->nil_obj();
        } else if (!size) {
            return (*this)[index];
        }
        size->assert_type(env, Value::Type::Integer, "Integer");
        size_t end = index + size->as_integer()->to_nat_int_t();
        size_t max = this->size();
        end = end > max ? max : end;
        ArrayValue *result = new ArrayValue { env };
        for (size_t i = index; i < end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        Value *begin_obj = range->begin();
        Value *end_obj = range->end();
        begin_obj->assert_type(env, Value::Type::Integer, "Integer");
        end_obj->assert_type(env, Value::Type::Integer, "Integer");
        nat_int_t begin = begin_obj->as_integer()->to_nat_int_t();
        nat_int_t end = end_obj->as_integer()->to_nat_int_t();
        if (begin < 0) {
            begin = this->size() + begin;
        }
        if (end < 0) {
            end = this->size() + end;
        }
        if (begin < 0 || end < 0) {
            if (begin_obj->as_integer()->is_zero()) {
                // NOTE: not entirely sure about this, but range beginning with 0..
                // seems to be a special case ¯\_(ツ)_/¯
                return new ArrayValue { env };
            }
            return env->nil_obj();
        }
        size_t u_end = static_cast<size_t>(end);
        if (!range->exclude_end()) u_end++;
        size_t max = this->size();
        u_end = u_end > max ? max : u_end;
        ArrayValue *result = new ArrayValue { env };
        for (size_t i = begin; i < u_end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else {
        env->raise("TypeError", "no implicit conversion of %s into Integer", index_obj->klass()->class_name());
    }
}

Value *ArrayValue::refeq(Env *env, Value *index_obj, Value *size, Value *val) {
    this->assert_not_frozen(env);
    index_obj->assert_type(env, Value::Type::Integer, "Integer"); // TODO: accept a range
    nat_int_t index = index_obj->as_integer()->to_nat_int_t();
    assert(index >= 0); // TODO: accept negative index
    size_t u_index = static_cast<size_t>(index);
    if (!val) {
        val = size;
        if (u_index < this->size()) {
            (*this)[u_index] = val;
        } else {
            expand_with_nil(env, u_index);
            push(val);
        }
        return val;
    }
    size->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t length = size->as_integer()->to_nat_int_t();
    assert(length >= 0);
    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayValue *ary2 = new ArrayValue { env };
    // stuff before the new entry/entries
    for (size_t i = 0; i < u_index; i++) {
        if (i >= this->size()) break;
        ary2->push((*this)[i]);
    }
    // extra nils if needed
    ary2->expand_with_nil(env, u_index);
    // the new entry/entries
    if (val->is_array()) {
        for (auto &v : *val->as_array()) {
            ary2->push(v);
        }
    } else {
        ary2->push(val);
    }
    // stuff after the new entry/entries
    for (size_t i = u_index + length; i < this->size(); i++) {
        ary2->push((*this)[i]);
    }
    overwrite(*ary2);
    return val;
}

Value *ArrayValue::any(Env *env, size_t argc, Value **args, Block *block) {
    ModuleValue *Enumerable = env->Object()->const_fetch("Enumerable")->as_module();
    return Enumerable->call_method(env, klass(), "any?", this, argc, args, block);
}

Value *ArrayValue::eq(Env *env, Value *other) {
    if (!other->is_array()) return env->false_obj();
    ArrayValue *other_array = other->as_array();
    if (size() != other_array->size()) return env->false_obj();
    if (size() == 0) return env->true_obj();
    for (size_t i = 0; i < size(); i++) {
        // TODO: could easily be optimized for strings and numbers
        Value *item = (*other_array)[i];
        Value *result = (*this)[i]->send(env, "==", 1, &item, nullptr);
        if (result->type() == Value::Type::False) return result;
    }
    return env->true_obj();
}

Value *ArrayValue::eql(Env *env, Value *other) {
    if (this == other)
        return env->true_obj();
    if (!other->is_array())
        return env->false_obj();

    auto other_array = other->as_array();
    if (size() != other_array->size())
        return env->false_obj();

    for (size_t i = 0; i < size(); ++i) {
        Value *item = (*other_array)[i];
        Value *result = (*this)[i]->send(env, "eql?", 1, &item, nullptr);
        if (result->type() == Value::Type::False)
            return result;
    }

    return env->true_obj();
}

Value *ArrayValue::each(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    for (auto &obj : *this) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, nullptr);
    }
    return this;
}

Value *ArrayValue::map(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        new_array->push(result);
    }
    return new_array;
}

// TODO: accept integer and return array
Value *ArrayValue::first(Env *env) {
    if (size() > 0) {
        return (*this)[0];
    } else {
        return env->nil_obj();
    }
}

Value *ArrayValue::sample(Env *env) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> random_number(1, size());

    if (size() > 0) {
        return (*this)[random_number(rng) - 1];
    } else {
        return env->nil_obj();
    }
}

// TODO: accept integer and return array
Value *ArrayValue::last(Env *env) {
    if (size() > 0) {
        return (*this)[size() - 1];
    } else {
        return env->nil_obj();
    }
}

Value *ArrayValue::include(Env *env, Value *item) {
    if (size() == 0) {
        return env->false_obj();
    } else {
        for (auto &compare_item : *this) {
            if (item->send(env, "==", 1, &compare_item, nullptr)->is_truthy()) {
                return env->true_obj();
            }
        }
        return env->false_obj();
    }
}

Value *ArrayValue::shift(Env *env, Value *count) {
    auto has_count = count != nullptr;
    size_t shift_count = 1;
    Value *result = nullptr;
    if (has_count) {
        count->assert_type(env, Value::Type::Integer, "Integer");
        shift_count = count->as_integer()->to_nat_int_t();
        if (shift_count == 0) {
            return new ArrayValue { env };
        }
        result = new ArrayValue { env, m_vector.slice(0, shift_count) };
    } else {
        result = m_vector[0];
    }

    auto shifted = m_vector.slice(shift_count);
    m_vector = std::move(shifted);
    return result;
}

Value *ArrayValue::sort(Env *env) {
    ArrayValue *copy = new ArrayValue { *this };
    copy->sort_in_place(env);
    return copy;
}

Value *ArrayValue::join(Env *env, Value *joiner) {
    if (size() == 0) {
        return new StringValue { env };
    } else if (size() == 1) {
        return (*this)[0]->send(env, "to_s");
    } else {
        if (!joiner) joiner = new StringValue { env, "" };
        joiner->assert_type(env, Value::Type::String, "String");
        StringValue *out = (*this)[0]->send(env, "to_s")->dup(env)->as_string();
        for (size_t i = 1; i < size(); i++) {
            Value *item = (*this)[i];
            out->append_string(env, joiner->as_string());
            out->append_string(env, item->send(env, "to_s")->as_string());
        }
        return out;
    }
}

Value *ArrayValue::cmp(Env *env, Value *other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *other_array = other->as_array();
    for (size_t i = 0; i < size(); i++) {
        if (i >= other_array->size()) {
            return new IntegerValue { env, 1 };
        }
        Value *item = (*other_array)[i];
        Value *cmp_obj = (*this)[i]->send(env, "<=>", 1, &item, nullptr);
        assert(cmp_obj->type() == Value::Type::Integer);
        nat_int_t cmp = cmp_obj->as_integer()->to_nat_int_t();
        if (cmp < 0) return new IntegerValue { env, -1 };
        if (cmp > 0) return new IntegerValue { env, 1 };
    }
    if (other_array->size() > size()) {
        return new IntegerValue { env, -1 };
    }
    return new IntegerValue { env, 0 };
}

void ArrayValue::push_splat(Env *env, Value *val) {
    if (!val->is_array() && val->respond_to(env, "to_a")) {
        val = val->send(env, "to_a");
    }
    if (val->is_array()) {
        for (Value *v : *val->as_array()) {
            push(*v);
        }
    } else {
        push(*val);
    }
}

Value *ArrayValue::pop(Env *env) {
    this->assert_not_frozen(env);
    if (size() == 0) return env->nil_obj();
    Value *val = m_vector[m_vector.size() - 1];
    m_vector.set_size(m_vector.size() - 1);
    return val;
}

void ArrayValue::expand_with_nil(Env *env, size_t total) {
    for (size_t i = size(); i < total; i++) {
        push(*env->nil_obj());
    }
}

void ArrayValue::sort_in_place(Env *env) {
    this->assert_not_frozen(env);
    auto cmp = [](void *env, Value *a, Value *b) {
        Value *compare = a->send(static_cast<Env *>(env), "<=>", 1, &b, nullptr);
        return compare->as_integer()->to_nat_int_t() < 0;
    };
    m_vector.sort(Vector<Value *>::SortComparator { env, cmp });
}

Value *ArrayValue::select(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy()) {
            new_array->push(item);
        }
    }
    return new_array;
}

Value *ArrayValue::max(Env *env) {
    if (m_vector.size() == 0)
        return env->nil_obj();
    Value *max = nullptr;
    for (auto item : *this) {
        if (!max || item->send(env, ">", 1, &max)->is_truthy())
            max = item;
    }
    return max;
}

Value *ArrayValue::min(Env *env) {
    if (m_vector.size() == 0)
        return env->nil_obj();
    Value *min = nullptr;
    for (auto item : *this) {
        if (!min || item->send(env, "<", 1, &min)->is_truthy())
            min = item;
    }
    return min;
}

}
