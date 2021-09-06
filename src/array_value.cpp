#include "natalie.hpp"
#include <algorithm>
#include <natalie/array_value.hpp>
#include <natalie/string_value.hpp>
#include <natalie/symbol_value.hpp>
#include <random>

namespace Natalie {

ValuePtr ArrayValue::initialize(Env *env, ValuePtr size, ValuePtr value, Block *block) {
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
    if (block) {
        for (nat_int_t i = 0; i < size->as_integer()->to_nat_int_t(); i++) {
            ValuePtr args = new IntegerValue { i };
            push(NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &args, nullptr));
        }
    } else {
        if (!value) value = NilValue::the();
        for (nat_int_t i = 0; i < size->as_integer()->to_nat_int_t(); i++) {
            push(value);
        }
    }
    return this;
}

ValuePtr ArrayValue::inspect(Env *env) {
    RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return new StringValue { "[...]" };

        StringValue *out = new StringValue { "[" };
        for (size_t i = 0; i < size(); i++) {
            ValuePtr obj = (*this)[i];

            auto inspected_repr = obj.send(env, SymbolValue::intern("inspect"));
            SymbolValue *to_s = SymbolValue::intern("to_s");

            if (!inspected_repr->is_string()) {
                if (inspected_repr->respond_to(env, to_s)) {
                    inspected_repr = obj.send(env, to_s);
                }
            }

            if (inspected_repr->is_string()) {
                out->append(env, inspected_repr->as_string());
            } else {
                char buf[1000];
                sprintf(buf, "#<%s:%p>", inspected_repr->klass()->class_name_or_blank()->c_str(), (void *)&inspected_repr);
                out->append(env, buf);
            }

            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(env, ']');
        return out;
    });
}

ValuePtr ArrayValue::ltlt(Env *env, ValuePtr arg) {
    this->assert_not_frozen(env);
    push(arg);
    return this;
}

ValuePtr ArrayValue::add(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { *this };
    new_array->concat(*other->as_array());
    return new_array;
}

ValuePtr ArrayValue::sub(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue {};
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other->as_array()) {
            if (item.send(env, SymbolValue::intern("=="), { compare_item })->is_truthy()) {
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

ValuePtr ArrayValue::sum(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto sum_method = Enumerable->find_method(env, SymbolValue::intern("sum"));
    return sum_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::ref(Env *env, ValuePtr index_obj, ValuePtr size) {
    auto sym_to_int = SymbolValue::intern("to_int");
    if (index_obj->respond_to(env, sym_to_int)) {
        index_obj = index_obj.send(env, sym_to_int);
    }
    if (index_obj->type() == Value::Type::Integer) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();
        if (index < 0) {
            index = this->size() + index;
        }
        if (index < 0) {
            return NilValue::the();
        }
        size_t sindex = static_cast<size_t>(index);
        if (sindex >= this->size()) {
            return NilValue::the();
        } else if (!size) {
            return (*this)[index];
        }
        size->assert_type(env, Value::Type::Integer, "Integer");
        size_t end = index + size->as_integer()->to_nat_int_t();
        size_t max = this->size();
        end = end > max ? max : end;
        ArrayValue *result = new ArrayValue {};
        for (size_t i = index; i < end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();
        ValuePtr end_obj = range->end();
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
        if (begin < 0 || end < 0 || (size_t(begin) > this->size())) {
            if (begin_obj->as_integer()->is_zero()) {
                // NOTE: not entirely sure about this, but range beginning with 0..
                // seems to be a special case ¯\_(ツ)_/¯
                return new ArrayValue {};
            }
            return NilValue::the();
        }
        size_t u_end = static_cast<size_t>(end);
        if (!range->exclude_end()) u_end++;
        size_t max = this->size();
        u_end = u_end > max ? max : u_end;
        ArrayValue *result = new ArrayValue {};
        for (size_t i = begin; i < u_end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else {
        // will throw
        index_obj->assert_type(env, ValueType::Integer, "Integer");
        return nullptr;
    }
}

ValuePtr ArrayValue::refeq(Env *env, ValuePtr index_obj, ValuePtr size, ValuePtr val) {
    this->assert_not_frozen(env);
    if (index_obj.is_integer()) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();
        if (index < 0) {
            if ((size_t)(-index) > this->size()) {
                env->raise("IndexError", "index {} too small for array; minimum: -{}", index, this->size());
                return nullptr;
            }
            index = this->size() + index;
        }
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
        if (length < 0) {
            env->raise("IndexError", "negative length ({})", length);
            return nullptr;
        }
        // PERF: inefficient for large arrays where changes are being made to only the right side
        ArrayValue *ary2 = new ArrayValue {};
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
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();
        ValuePtr end_obj = range->end();
        begin_obj->assert_type(env, Value::Type::Integer, "Integer");
        end_obj->assert_type(env, Value::Type::Integer, "Integer");

        // ignore "size"
        val = size;

        nat_int_t start = begin_obj.to_nat_int_t();

        if (start < 0) {
            if ((size_t)(-start) > this->size()) {
                env->raise("RangeError", "{}..{}{} out of range", start, range->exclude_end() ? "." : "", end_obj.to_nat_int_t());
                return nullptr;
            }
            start = this->size() + start;
        }

        nat_int_t length = end_obj.to_nat_int_t() - start + (range->exclude_end() ? 0 : 1);

        if (length < 0) {
            length = 0;
        }
        if (length + start > (nat_int_t)this->size()) {
            length = this->size() - start;
        }

        // PERF: inefficient for large arrays where changes are being made to only the right side
        ArrayValue *ary2 = new ArrayValue {};
        // stuff before the new entry/entries
        for (nat_int_t i = 0; i < start; i++) {
            if (i >= (nat_int_t)this->size()) break;
            ary2->push((*this)[i]);
        }

        // extra nils if needed
        ary2->expand_with_nil(env, start);

        // the new entry/entries
        if (val->is_array()) {
            for (auto &v : *val->as_array()) {
                ary2->push(v);
            }
        } else {
            ary2->push(val);
        }

        // stuff after the new entry/entries
        for (size_t i = start + length; i < this->size(); i++) {
            ary2->push((*this)[i]);
        }

        overwrite(*ary2);
    } else {
        // will throw
        index_obj->assert_type(env, ValueType::Integer, "Integer");
        return nullptr;
    }
    return val;
}

ValuePtr ArrayValue::any(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: deletating to Enumerable#any? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#any? in MRI and it won't affect Array#any?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto any_method = Enumerable->find_method(env, SymbolValue::intern("any?"));
    return any_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::eq(Env *env, ValuePtr other) {
    RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) -> ValuePtr {
        if (other == this)
            return TrueValue::the();

        SymbolValue *equality = SymbolValue::intern("==");
        if (!other->is_array()
            && other->send(env, SymbolValue::intern("respond_to?"), { SymbolValue::intern("to_ary") })->is_true())
            return other->send(env, equality, { this });

        if (!other->is_array()) {
            return FalseValue::the();
        }

        auto other_array = other->as_array();
        if (size() != other_array->size())
            return FalseValue::the();

        if (is_recursive)
            //since == is an & of all the == of each value, this will just leave the expression uneffected
            return TrueValue::the();

        SymbolValue *object_id = SymbolValue::intern("object_id");
        for (size_t i = 0; i < size(); ++i) {
            ValuePtr this_item = (*this)[i];
            ValuePtr item = (*other_array)[i];

            if (this_item->respond_to(env, object_id) && item->respond_to(env, object_id)) {
                ValuePtr same_object_id = this_item
                                              ->send(env, object_id)
                                              ->send(env, equality, { item->send(env, object_id) });

                // This allows us to check NAN equality and other potentially similar constants
                if (same_object_id->is_true())
                    continue;
            }

            ValuePtr result = this_item.send(env, equality, 1, &item, nullptr);
            if (result->is_false())
                return result;
        }

        return TrueValue::the();
    });
}

ValuePtr ArrayValue::eql(Env *env, ValuePtr other) {
    RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) -> ValuePtr {
        if (other == this)
            return TrueValue::the();
        if (!other->is_array())
            return FalseValue::the();

        if (!other->is_array()) {
            return FalseValue::the();
        }

        auto other_array = other->as_array();
        if (size() != other_array->size())
            return FalseValue::the();

        if (is_recursive)
            //since eql is an & of all the eql of each value, this will just leave the expression uneffected
            return TrueValue::the();

        for (size_t i = 0; i < size(); ++i) {
            ValuePtr item = (*other_array)[i];
            ValuePtr result = (*this)[i].send(env, SymbolValue::intern("eql?"), 1, &item, nullptr);
            if (result->is_false())
                return result;
        }

        return TrueValue::the();
    });
}

ValuePtr ArrayValue::each(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each") });

    for (auto &obj : *this) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, nullptr);
    }
    return this;
}

ValuePtr ArrayValue::map(Env *env, Block *block) {
    env->ensure_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue {};
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        new_array->push(result);
    }
    return new_array;
}

ValuePtr ArrayValue::first(Env *env, ValuePtr n) {
    auto has_count = n != nullptr;

    if (!has_count) {
        if (is_empty())
            return NilValue::the();

        return (*this)[0];
    }

    ArrayValue *array = new ArrayValue();

    n->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "negative array size");
        return nullptr;
    }

    size_t end = std::min(size(), (size_t)n_value);

    for (size_t k = 0; k < end; ++k) {
        array->push((*this)[k]);
    }

    return array;
}

ValuePtr ArrayValue::flatten(Env *env, ValuePtr depth) {
    ArrayValue *copy = new ArrayValue { *this };
    copy->flatten_in_place(env, depth);
    return copy;
}

ValuePtr ArrayValue::flatten_in_place(Env *env, ValuePtr depth) {
    this->assert_not_frozen(env);

    bool changed { false };

    auto has_depth = depth != nullptr;
    if (has_depth) {
        auto sym_to_i = SymbolValue::intern("to_i");
        auto sym_to_int = SymbolValue::intern("to_int");
        if (!depth->is_integer()) {
            if (depth->respond_to(env, sym_to_i)) {
                depth = depth.send(env, sym_to_i);
            } else if (depth->respond_to(env, sym_to_int)) {
                depth = depth.send(env, sym_to_int);
            } else {
                env->raise("TypeError", "no implicit conversion of {} into Integer", depth->klass()->class_name_or_blank());
                return nullptr;
            }
        }

        depth->assert_type(env, Value::Type::Integer, "Integer");
        nat_int_t depth_value = depth->as_integer()->to_nat_int_t();

        changed = _flatten_in_place(env, depth_value);
    } else {
        changed = _flatten_in_place(env, -1);
    }

    if (changed)
        return this;

    return NilValue::the();
}

bool ArrayValue::_flatten_in_place(Env *env, nat_int_t depth, Hashmap<ArrayValue *> visited) {
    bool changed { false };

    if (visited.is_empty())
        visited.set(this);

    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];

        if (depth == 0 || item == nullptr) {
            continue;
        }

        if (!item->is_array()) {
            auto sym_to_ary = SymbolValue::intern("to_ary");
            auto sym_to_a = SymbolValue::intern("to_a");
            auto original_item_class = item->klass();
            ValuePtr new_item;

            if (item->respond_to(env, sym_to_ary)) {
                new_item = item.send(env, sym_to_ary);
            } else if (item->respond_to(env, sym_to_a)) {
                new_item = item.send(env, sym_to_a);
            }

            if (new_item != nullptr && !new_item->is_nil()) {
                if (!new_item->is_array()) {
                    auto original_item_class_name = original_item_class->class_name_or_blank();
                    auto new_item_class_name = item->klass()->class_name_or_blank();
                    env->raise(
                        "TypeError",
                        "can't convert {} to Array ({}#to_ary gives {})",
                        original_item_class_name,
                        original_item_class_name,
                        new_item_class_name);
                    return false;
                }

                item = new_item;
            }
        }

        if (item->is_array()) {
            changed = true;
            m_vector.remove(i - 1);

            auto array_item = item->as_array();

            if (visited.get(array_item) != nullptr) {
                env->raise("ArgumentError", "tried to flatten recursive array");
                return false;
            }

            // add current array item so if it is referenced by any of its children a loop is detected
            visited.set(array_item);

            // use a copy so we avoid altering the content of nested arrays
            ArrayValue *copy = new ArrayValue { *array_item };

            copy->_flatten_in_place(env, depth - 1, visited);
            for (size_t j = 0; j < copy->size(); ++j) {
                m_vector.insert(i + j - 1, (*copy)[j]);
            }

            // remove current array item as same reference siblings are valid
            visited.remove(array_item);
        }
    }

    return changed;
}

ValuePtr ArrayValue::dig(Env *env, size_t argc, ValuePtr *args) {
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

ValuePtr ArrayValue::drop(Env *env, ValuePtr n) {
    n->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "attempt to drop negative size");
        return nullptr;
    }

    ArrayValue *array = new ArrayValue();
    for (size_t k = n_value; k < size(); ++k) {
        array->push((*this)[k]);
    }

    return array;
}

ValuePtr ArrayValue::last(Env *env, ValuePtr n) {
    auto has_count = n != nullptr;

    if (!has_count) {
        if (is_empty())
            return NilValue::the();

        return (*this)[size() - 1];
    }

    ArrayValue *array = new ArrayValue();

    n->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "negative array size");
        return nullptr;
    }

    assert(size() <= NAT_INT_MAX);
    nat_int_t signed_size = static_cast<nat_int_t>(size());
    for (size_t k = std::max(static_cast<nat_int_t>(0), signed_size - n_value); k < size(); ++k) {
        array->push((*this)[k]);
    }

    return array;
}

ValuePtr ArrayValue::sample(Env *env) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> random_number(1, size());

    if (size() > 0) {
        return (*this)[random_number(rng) - 1];
    } else {
        return NilValue::the();
    }
}

ValuePtr ArrayValue::include(Env *env, ValuePtr item) {
    if (size() == 0) {
        return FalseValue::the();
    } else {
        for (auto &compare_item : *this) {
            if (item.send(env, SymbolValue::intern("=="), { compare_item })->is_truthy()) {
                return TrueValue::the();
            }
        }
        return FalseValue::the();
    }
}

ValuePtr ArrayValue::index(Env *env, ValuePtr object, Block *block) {
    assert(size() <= NAT_INT_MAX);
    auto length = static_cast<nat_int_t>(size());
    if (block) {
        for (nat_int_t i = 0; i < length; i++) {
            auto item = m_vector[i];
            ValuePtr args[] = { item };
            auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
            if (result->is_truthy())
                return ValuePtr::integer(i);
        }
        return NilValue::the();
    } else if (object) {
        for (nat_int_t i = 0; i < length; i++) {
            auto item = m_vector[i];
            if (item.send(env, SymbolValue::intern("=="), { object })->is_truthy())
                return ValuePtr::integer(i);
        }
        return NilValue::the();
    } else {
        // TODO
        env->ensure_block_given(block);
        NAT_UNREACHABLE();
    }
}

ValuePtr ArrayValue::shift(Env *env, ValuePtr count) {
    auto has_count = count != nullptr;
    size_t shift_count = 1;
    ValuePtr result = nullptr;
    if (has_count) {
        count->assert_type(env, Value::Type::Integer, "Integer");
        shift_count = count->as_integer()->to_nat_int_t();
        if (shift_count == 0) {
            return new ArrayValue {};
        }
        result = new ArrayValue { m_vector.slice(0, shift_count) };
    } else {
        result = m_vector[0];
    }

    auto shifted = m_vector.slice(shift_count);
    m_vector = std::move(shifted);
    return result;
}

ValuePtr ArrayValue::sort(Env *env, Block *block) {
    ArrayValue *copy = new ArrayValue { *this };
    copy->sort_in_place(env, block);
    return copy;
}

ValuePtr ArrayValue::join(Env *env, ValuePtr joiner) {
    if (size() == 0) {
        return new StringValue {};
    } else if (size() == 1) {
        return (*this)[0].send(env, SymbolValue::intern("to_s"));
    } else {
        if (!joiner) joiner = new StringValue { "" };
        joiner->assert_type(env, Value::Type::String, "String");
        StringValue *out = (*this)[0].send(env, SymbolValue::intern("to_s"))->dup(env)->as_string();
        for (size_t i = 1; i < size(); i++) {
            ValuePtr item = (*this)[i];
            out->append(env, joiner->as_string());
            out->append(env, item.send(env, SymbolValue::intern("to_s"))->as_string());
        }
        return out;
    }
}

ValuePtr ArrayValue::cmp(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *other_array = other->as_array();
    for (size_t i = 0; i < size(); i++) {
        if (i >= other_array->size()) {
            return ValuePtr::integer(1);
        }
        ValuePtr item = (*other_array)[i];
        ValuePtr cmp_obj = (*this)[i].send(env, SymbolValue::intern("<=>"), { item });
        assert(cmp_obj->type() == Value::Type::Integer);
        nat_int_t cmp = cmp_obj->as_integer()->to_nat_int_t();
        if (cmp < 0) return ValuePtr::integer(-1);
        if (cmp > 0) return ValuePtr::integer(1);
    }
    if (other_array->size() > size()) {
        return ValuePtr::integer(-1);
    }
    return ValuePtr::integer(0);
}

ValuePtr ArrayValue::push(Env *env, size_t argc, ValuePtr *args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < argc; i++) {
        push(args[i]);
    }
    return this;
}

void ArrayValue::push_splat(Env *env, ValuePtr val) {
    if (!val->is_array() && val->respond_to(env, SymbolValue::intern("to_a"))) {
        val = val.send(env, SymbolValue::intern("to_a"));
    }
    if (val->is_array()) {
        for (ValuePtr v : *val->as_array()) {
            push(*v);
        }
    } else {
        push(*val);
    }
}

ValuePtr ArrayValue::pop(Env *env) {
    this->assert_not_frozen(env);
    if (size() == 0) return NilValue::the();
    ValuePtr val = m_vector[m_vector.size() - 1];
    m_vector.set_size(m_vector.size() - 1);
    return val;
}

void ArrayValue::expand_with_nil(Env *env, size_t total) {
    for (size_t i = size(); i < total; i++) {
        push(*NilValue::the());
    }
}

bool array_sort_compare(Env *env, ValuePtr a, ValuePtr b, Block *block) {
    if (block) {
        ValuePtr args[2] = { a, b };
        ValuePtr compare = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr);

        if (compare->respond_to(env, SymbolValue::intern("<"))) {
            ValuePtr zero = ValuePtr::integer(0);
            return compare.send(env, SymbolValue::intern("<"), { zero })->is_truthy();
        } else {
            env->raise("ArgumentError", "comparison of {} with 0 failed", compare->klass()->class_name_or_blank());
        }
    } else {
        ValuePtr compare = a.send(env, SymbolValue::intern("<=>"), { b });
        if (compare->is_integer()) {
            return compare->as_integer()->to_nat_int_t() < 0;
        }
        // TODO: Ruby sometimes prints b as the value (for example for integers) and sometimes as class
        env->raise("ArgumentError", "comparison of {} with {} failed", a->klass()->class_name_or_blank(), b->klass()->class_name_or_blank());
    }
}

ValuePtr ArrayValue::sort_in_place(Env *env, Block *block) {
    this->assert_not_frozen(env);

    m_vector.sort([env, block](ValuePtr a, ValuePtr b) {
        return array_sort_compare(env, a, b, block);
    });

    return this;
}

bool array_sort_by_compare(Env *env, ValuePtr a, ValuePtr b, Block *block) {
    ValuePtr a_res = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &a, nullptr);
    ValuePtr b_res = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &b, nullptr);

    ValuePtr compare = a_res.send(env, SymbolValue::intern("<=>"), { b_res });
    if (compare->is_integer()) {
        return compare->as_integer()->to_nat_int_t() < 0;
    }
    env->raise("ArgumentError", "comparison of {} with {} failed", a_res->klass()->class_name_or_blank(), b_res->klass()->class_name_or_blank());
}

ValuePtr ArrayValue::sort_by_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("sort_by!") });

    this->assert_not_frozen(env);

    m_vector.sort([env, block](ValuePtr a, ValuePtr b) {
        return array_sort_by_compare(env, a, b, block);
    });

    return this;
}

ValuePtr ArrayValue::select(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("select") });

    ArrayValue *new_array = new ArrayValue {};
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy()) {
            new_array->push(item);
        }
    }
    return new_array;
}

ValuePtr ArrayValue::reject(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("reject") });

    ArrayValue *new_array = new ArrayValue {};
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_falsey()) {
            new_array->push(item);
        }
    }
    return new_array;
}

ValuePtr ArrayValue::max(Env *env) {
    if (m_vector.size() == 0)
        return NilValue::the();
    ValuePtr max = nullptr;
    for (auto item : *this) {
        if (!max || item.send(env, SymbolValue::intern(">"), { max })->is_truthy())
            max = item;
    }
    return max;
}

ValuePtr ArrayValue::min(Env *env) {
    if (m_vector.size() == 0)
        return NilValue::the();
    ValuePtr min = nullptr;
    for (auto item : *this) {
        if (!min || item.send(env, SymbolValue::intern("<"), { min })->is_truthy())
            min = item;
    }
    return min;
}

ValuePtr ArrayValue::compact(Env *env) {
    auto ary = new ArrayValue {};
    for (auto item : *this) {
        if (item->is_nil()) continue;
        ary->push(item);
    }
    return ary;
}

ValuePtr ArrayValue::compact_in_place(Env *env) {
    this->assert_not_frozen(env);

    bool changed { false };
    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];
        if (item->is_nil()) {
            changed = true;
            m_vector.remove(i - 1);
        }
    }

    if (changed)
        return this;

    return NilValue::the();
}

ValuePtr ArrayValue::uniq_in_place(Env *env, Block *block) {
    this->assert_not_frozen(env);

    auto hash = new HashValue {};
    for (auto item : *this) {
        ValuePtr key = item;
        if (block) {
            key = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &item, nullptr);
        }
        if (hash->has_key(env, key)->is_false()) {
            hash->put(env, key, item);
        }
    }

    ArrayValue *values = hash->values(env)->as_array();

    if (m_vector.size() == values->size())
        return NilValue::the();

    m_vector.clear();
    for (auto item : *values) {
        m_vector.push(item);
    }

    return this;
}

ValuePtr ArrayValue::clear(Env *env) {
    this->assert_not_frozen(env);
    m_vector.clear();
    return this;
}

ValuePtr ArrayValue::at(Env *env, ValuePtr n) {
    return ref(env, n, nullptr);
}

ValuePtr ArrayValue::assoc(Env *env, ValuePtr needle) {
    // TODO use common logic for this (see for example rassoc and index)
    for (auto &item : *this) {
        if (!item->is_array())
            continue;

        ArrayValue *sub_array = item->as_array();
        if (sub_array->is_empty())
            continue;

        if (needle.send(env, SymbolValue::intern("=="), { (*sub_array)[0] })->is_truthy())
            return sub_array;
    }

    return NilValue::the();
}

ValuePtr ArrayValue::rassoc(Env *env, ValuePtr needle) {
    for (auto &item : *this) {
        if (!item->is_array())
            continue;

        ArrayValue *sub_array = item->as_array();
        if (sub_array->size() < 2)
            continue;

        if (needle.send(env, SymbolValue::intern("=="), { (*sub_array)[1] })->is_truthy())
            return sub_array;
    }

    return NilValue::the();
}

ValuePtr ArrayValue::insert(Env *env, size_t argc, ValuePtr *args) {
    this->assert_not_frozen(env);

    if (argc == 1)
        return this;

    auto index_ptr = args[0];

    if (!index_ptr.is_integer()) {
        auto sym_to_int = SymbolValue::intern("to_int");

        if (index_ptr->respond_to(env, sym_to_int)) {
            index_ptr = index_ptr->send(env, sym_to_int);
        }
    }

    if (!index_ptr.is_integer())
        env->raise("TypeError", "no implicit conversion of {} into Integer", index_ptr->klass()->class_name_or_blank());

    auto index = index_ptr.to_nat_int_t();
    auto should_append = index < 0;

    if (index < 0)
        index += size();

    if (should_append)
        index += 1;

    if (index < 0) {
        env->raise("IndexError", "index {} too small for array; minimum: -{}",
            index,
            m_vector.size() + 1);
    }

    size_t size_t_index = static_cast<size_t>(index);

    while (size_t_index > m_vector.size()) {
        m_vector.push(NilValue::the());
    }

    for (size_t i = 1; i < argc; i++) {
        m_vector.insert(i + size_t_index - 1, args[i]);
    }

    return this;
}

ValuePtr ArrayValue::intersection(Env *env, ValuePtr arg) {
    if (!arg->is_array()) {
        env->raise("TypeError", "no implicit conversion of {} into Array", arg->klass()->class_name_or_blank());
        return nullptr;
    }

    auto *other_array = arg->as_array();

    if (is_empty() || other_array->is_empty()) {
        return new ArrayValue();
    }

    auto *result = new ArrayValue();
    for (auto &val : *this) {
        if (other_array->include(env, val)->is_truthy()) {
            result->push(val);
        }
    }

    return result;
}

ValuePtr ArrayValue::intersection(Env *env, size_t argc, ValuePtr *args) {
    auto *result = new ArrayValue(env, *this);

    // TODO: we probably want to make & call this instead of this way for optimization
    for (size_t i = 0; i < argc; i++) {
        auto &arg = args[i];
        result = result->intersection(env, arg)->as_array();
    }

    return result;
}

ValuePtr ArrayValue::union_of(Env *env, ValuePtr arg) {
    if (!arg->is_array()) {
        env->raise("TypeError", "no implicit conversion of {} into Array", arg->klass()->class_name_or_blank());
        return nullptr;
    }

    auto *result = new ArrayValue();
    auto add_value = [&result, &env](ValuePtr &val) {
        if (result->include(env, val)->is_falsey()) {
            result->push(val);
        }
    };

    for (auto &val : *this) {
        add_value(val);
    }

    auto *other_array = arg->as_array();
    for (auto &val : *other_array) {
        add_value(val);
    }

    return result;
}

ValuePtr ArrayValue::union_of(Env *env, size_t argc, ValuePtr *args) {
    auto *result = new ArrayValue(env, *this);

    // TODO: we probably want to make | call this instead of this way for optimization
    for (size_t i = 0; i < argc; i++) {
        auto &arg = args[i];
        result = result->union_of(env, arg)->as_array();
    }

    return result;
}

ValuePtr ArrayValue::reverse(Env *env) {
    ArrayValue *copy = new ArrayValue(env, *this);
    copy->reverse_in_place(env);
    return copy;
}

ValuePtr ArrayValue::reverse_each(Env *env, Block *block) {
    if (!block) {
        auto enumerator = send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("reverse_each") });
        enumerator->ivar_set(env, SymbolValue::intern("@size"), ValuePtr::integer(m_vector.size()));
        return enumerator;
    }

    for (size_t i = m_vector.size(); i > 0; --i) {
        auto obj = (*this)[i - 1];
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, nullptr);
    }

    return this;
}

ValuePtr ArrayValue::reverse_in_place(Env *env) {
    assert_not_frozen(env);

    for (size_t i = 0; i < size() / 2; i++) {
        std::swap(m_vector[i], m_vector[size() - 1 - i]);
    }
    return this;
}

ValuePtr ArrayValue::concat(Env *env, size_t argc, ValuePtr *args) {
    assert_not_frozen(env);

    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];
        if (!arg->is_array()) {
            env->raise("TypeError", "no implicit conversion of {} into Array", arg->klass()->class_name_or_blank());
            return nullptr;
        }

        concat(*arg->as_array());
    }

    return this;
}

ValuePtr ArrayValue::rindex(Env *env, ValuePtr object, Block *block) {
    // TODO make dry since this code is almost identical to index
    assert(size() <= NAT_INT_MAX);
    auto length = static_cast<nat_int_t>(size());
    if (block) {
        for (nat_int_t i = length - 1; i >= 0; i--) {
            auto item = m_vector[i];
            ValuePtr args[] = { item };
            auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
            if (result->is_truthy())
                return ValuePtr::integer(i);
        }
        return NilValue::the();
    } else if (object) {
        for (nat_int_t i = length - 1; i >= 0; i--) {
            auto item = m_vector[i];
            if (item.send(env, SymbolValue::intern("=="), { object })->is_truthy())
                return ValuePtr::integer(i);
        }
        return NilValue::the();
    } else {
        // TODO
        env->ensure_block_given(block);
        NAT_UNREACHABLE();
    }
}

ValuePtr ArrayValue::none(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: deletating to Enumerable#none? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#none? in MRI and it won't affect Array#none?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto none_method = Enumerable->find_method(env, SymbolValue::intern("none?"));
    return none_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::one(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: deletating to Enumerable#one? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#one? in MRI and it won't affect Array#one?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto one_method = Enumerable->find_method(env, SymbolValue::intern("one?"));
    return one_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::rotate(Env *env, ValuePtr val) {
    ArrayValue *copy = new ArrayValue(env, *this);
    copy->rotate_in_place(env, val);
    return copy;
}

ValuePtr ArrayValue::rotate_in_place(Env *env, ValuePtr val) {
    assert_not_frozen(env);
    nat_int_t count = 1;
    if (val) {
        if (!val->is_integer()) {
            env->raise("TypeError", "no implicit conversion of {} into Integer", val->klass()->class_name_or_blank());
            return nullptr;
        }
        count = val->as_integer()->to_nat_int_t();
    }

    if (size() == 0) {
        // prevent
        return this;
    }

    if (count == 0) {
        return this;
    }

    if (count > 0) {
        Vector<ValuePtr> stack;

        count = count % size();

        for (nat_int_t i = 0; i < count; i++)
            stack.push(m_vector.pop_front());

        for (auto &rotated_val : stack)
            push(rotated_val);

    } else if (count < 0) {
        Vector<ValuePtr> stack;

        count = -count % size();
        for (nat_int_t i = 0; i < count; i++)
            stack.push(m_vector.pop());

        for (auto &rotated_val : stack)
            m_vector.push_front(rotated_val);
    }

    return this;
}

ValuePtr ArrayValue::slice_in_place(Env *env, ValuePtr index_obj, ValuePtr size) {
    if (size) {
        // a second argument means we must! take the integer branch
        index_obj->assert_type(env, ValueType::Integer, "Integer");
    }

    if (index_obj->is_integer()) {
        nat_int_t val = index_obj.to_nat_int_t();

        if (val < 0 || val >= (nat_int_t)this->size()) {
            return NilValue::the();
        }

        if (!size) {
            ValuePtr item = (*this)[val];
            for (size_t i = val; i < this->size() - 1; i++) {
                (*this)[i] = (*this)[i + 1];
            }
            m_vector.pop();
            return item;
        }
        size->assert_type(env, ValueType::Integer, "Integer");

        nat_int_t length = size.to_nat_int_t();

        if (length < 0) {
            return NilValue::the();
        }

        ArrayValue *newArr = new ArrayValue();
        if (length == 0) {
            return newArr;
        }

        if (val + length > (nat_int_t)this->size()) {
            length = this->size() - val;
        }

        for (nat_int_t i = val; i < val + length; i++) {
            newArr->push((*this)[i]);
        }

        for (nat_int_t i = val + length; i < (nat_int_t)this->size(); i++) {
            (*this)[i - length] = (*this)[i];
        }

        for (nat_int_t i = 0; i < length; i++) {
            m_vector.pop();
        }

        return newArr;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();
        ValuePtr end_obj = range->end();
        begin_obj->assert_type(env, Value::Type::Integer, "Integer");
        end_obj->assert_type(env, Value::Type::Integer, "Integer");

        nat_int_t start = begin_obj.to_nat_int_t();

        if (start < 0) {
            if ((size_t)(-start) > this->size()) {
                return NilValue::the();
            }
            start = this->size() + start;
        }

        nat_int_t length = end_obj.to_nat_int_t() - start + (range->exclude_end() ? 0 : 1);

        if (length < 0) {
            length = 0;
        }
        if (length + start > (nat_int_t)this->size()) {
            length = this->size() - start;
        }

        if (length < 0) {
            return NilValue::the();
        }

        ArrayValue *newArr = new ArrayValue();
        if (length == 0) {
            return newArr;
        }

        if (start + length > (nat_int_t)this->size()) {
            length = this->size() - start;
        }

        for (nat_int_t i = start; i < start + length; i++) {
            newArr->push((*this)[i]);
        }

        for (nat_int_t i = start + length; i < (nat_int_t)this->size(); i++) {
            (*this)[i - length] = (*this)[i];
        }

        for (nat_int_t i = 0; i < length; i++) {
            m_vector.pop();
        }

        return newArr;
    } else {
        // will throw
        index_obj->assert_type(env, ValueType::Integer, "Integer");
        return nullptr;
    }
}

ValuePtr ArrayValue::to_h(Env *env, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto to_h_method = Enumerable->find_method(env, SymbolValue::intern("to_h"));
    return to_h_method->call(env, this, 0, nullptr, block);
}

ValuePtr ArrayValue::try_convert(Env *env, ValuePtr val) {
    auto to_ary = SymbolValue::intern("to_ary");
    if (!val->respond_to(env, to_ary)) {
        return NilValue::the();
    }

    auto conversion = val->send(env, to_ary);

    if (conversion->is_array() || conversion->is_nil()) {
        return conversion;
    }

    auto original_item_class_name = val->klass()->class_name_or_blank();
    auto new_item_class_name = conversion->klass()->class_name_or_blank();
    env->raise(
        "TypeError",
        "can't convert {} to Array ({}#to_ary gives {})",
        original_item_class_name,
        original_item_class_name,
        new_item_class_name);
}

ValuePtr ArrayValue::zip(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto zip_method = Enumerable->find_method(env, SymbolValue::intern("zip"));
    return zip_method->call(env, this, argc, args, block);
}

}
