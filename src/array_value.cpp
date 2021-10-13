#include "natalie.hpp"
#include <algorithm>
#include <math.h>
#include <natalie/array_packer/packer.hpp>
#include <natalie/array_value.hpp>
#include <natalie/string_value.hpp>
#include <natalie/symbol_value.hpp>
#include <random>
#include <tm/hashmap.hpp>
#include <tm/recursion_guard.hpp>

namespace Natalie {

ValuePtr ArrayValue::allocate(Env *env, size_t argc, ValuePtr *args) {
    env->ensure_argc_is(argc, 0);
    return new ArrayValue {};
}

ValuePtr ArrayValue::initialize(Env *env, ValuePtr size, ValuePtr value, Block *block) {
    this->assert_not_frozen(env);

    if (!size) {
        ArrayValue new_array;
        *this = std::move(new_array);
        return this;
    }

    if (!value) {
        auto to_ary = SymbolValue::intern("to_ary");
        if (!size->is_array() && size->respond_to(env, to_ary))
            size = size->send(env, to_ary);

        if (size->is_array()) {
            auto target = *size->as_array();
            *this = std::move(target);
            return this;
        }
    }

    auto to_int = SymbolValue::intern("to_int");
    if (!size.is_integer() && size->respond_to(env, to_int))
        size = size->send(env, to_int);
    size->assert_type(env, Value::Type::Integer, "Integer");

    auto s = size->as_integer()->to_nat_int_t();

    if (s < 0)
        env->raise("ArgumentError", "negative argument");

    if (block) {
        if (value)
            env->warn("block supersedes default value argument");

        ArrayValue new_array;
        *this = std::move(new_array);

        for (nat_int_t i = 0; i < s; i++) {
            ValuePtr args = new IntegerValue { i };
            push(NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &args, nullptr));
        }

    } else {
        if (!value) value = NilValue::the();
        for (nat_int_t i = 0; i < s; i++) {
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
    auto to_ary = SymbolValue::intern("to_ary");
    if (!other->is_array() && other->respond_to(env, to_ary))
        other = other.send(env, to_ary);

    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { *this };
    new_array->concat(*other->as_array());
    return new_array;
}

ValuePtr ArrayValue::sub(Env *env, ValuePtr other) {
    auto to_ary = SymbolValue::intern("to_ary");
    if (!other->is_array() && other->respond_to(env, to_ary))
        other = other.send(env, to_ary);
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue {};
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other->as_array()) {
            if ((item.send(env, SymbolValue::intern("eql?"), { compare_item })->is_truthy() && item.send(env, SymbolValue::intern("hash")) == compare_item.send(env, SymbolValue::intern("hash")))
                || item.send(env, SymbolValue::intern("=="), { compare_item })->is_truthy()) {
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
    if (!size && index_obj->is_integer()) {
        auto index = _resolve_index(index_obj->as_integer()->to_nat_int_t());
        if (index < 0 || index >= (nat_int_t)m_vector.size())
            return NilValue::the();
        return m_vector[index];
    }

    ArrayValue *copy = new ArrayValue { *this };
    return copy->slice_in_place(env, index_obj, size);
}

ValuePtr ArrayValue::refeq(Env *env, ValuePtr index_obj, ValuePtr size, ValuePtr val) {
    this->assert_not_frozen(env);
    auto to_int = SymbolValue::intern("to_int");
    nat_int_t start, width;
    if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();
        ValuePtr end_obj = range->end();

        // Ignore "size"
        val = size;

        if (!begin_obj.is_integer() && begin_obj->respond_to(env, to_int))
            begin_obj = begin_obj->send(env, to_int);
        begin_obj->assert_type(env, Value::Type::Integer, "Integer");

        if (!end_obj.is_integer() && end_obj->respond_to(env, to_int))
            end_obj = end_obj->send(env, to_int);
        end_obj->assert_type(env, Value::Type::Integer, "Integer");

        start = begin_obj.to_nat_int_t();
        if (start < 0) {
            if ((size_t)(-start) > this->size()) {
                env->raise("RangeError", "{}..{}{} out of range", start, range->exclude_end() ? "." : "", end_obj.to_nat_int_t());
                return nullptr;
            }
            start = this->size() + start;
        }

        nat_int_t end = end_obj.to_nat_int_t();
        if (end < 0) {
            end = this->size() + end;
        }

        width = end - start + (range->exclude_end() ? 0 : 1);
        if (width < 0) {
            width = 0;
        }
        if (width + start > (nat_int_t)this->size()) {
            width = this->size() - start;
        }
    } else {
        if (!index_obj.is_integer() && index_obj->respond_to(env, to_int))
            index_obj = index_obj->send(env, to_int);
        index_obj->assert_type(env, ValueType::Integer, "Integer");

        start = index_obj->as_integer()->to_nat_int_t();
        if (start < 0) {
            if ((size_t)(-start) > this->size()) {
                env->raise("IndexError", "index {} too small for array; minimum: -{}", start, this->size());
                return nullptr;
            }
            start = this->size() + start;
        }

        if (!val) {
            val = size;
            if (start < (nat_int_t)this->size()) {
                (*this)[start] = val;
            } else {
                expand_with_nil(env, start);
                push(val);
            }
            return val;
        }

        if (!size.is_integer() && size->respond_to(env, to_int))
            size = size->send(env, to_int);

        size->assert_type(env, Value::Type::Integer, "Integer");
        width = size->as_integer()->to_nat_int_t();
        if (width < 0) {
            env->raise("IndexError", "negative length ({})", width);
            return nullptr;
        }
    }

    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayValue *new_ary = new ArrayValue {};

    // stuff before the new entry/entries
    for (size_t i = 0; i < (size_t)start; i++) {
        if (i >= this->size()) break;
        new_ary->push((*this)[i]);
    }

    // extra nils if needed
    new_ary->expand_with_nil(env, start);

    // the new entry/entries
    auto to_ary = SymbolValue::intern("to_ary");
    if (val->is_array() || val->respond_to(env, to_ary)) {
        if (!val->is_array())
            val = val.send(env, to_ary);

        val->assert_type(env, Value::Type::Array, "Array");

        for (auto &v : *val->as_array()) {
            new_ary->push(v);
        }
    } else {
        new_ary->push(val);
    }

    // stuff after the new entry/entries
    for (size_t i = start + width; i < this->size(); ++i) {
        new_ary->push((*this)[i]);
    }

    overwrite(*new_ary);

    return val;
}

ValuePtr ArrayValue::any(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: delegating to Enumerable#any? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#any? in MRI and it won't affect Array#any?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto any_method = Enumerable->find_method(env, SymbolValue::intern("any?"));
    return any_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::eq(Env *env, ValuePtr other) {
    TM::PairedRecursionGuard guard { this, other.value() };

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
    TM::PairedRecursionGuard guard { this, other.value() };

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

    for (size_t i = 0; i < size(); ++i) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &(*this)[i], nullptr);
    }
    return this;
}

ValuePtr ArrayValue::each_index(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each_index") });

    nat_int_t size_nat_int_t = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < size_nat_int_t; i++) {
        ValuePtr args = new IntegerValue { i };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &args, nullptr);
    }
    return this;
}

ValuePtr ArrayValue::map(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("map") });

    ArrayValue *copy = new ArrayValue { *this };
    copy->map_in_place(env, block);
    return copy;
}

ValuePtr ArrayValue::map_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("map!") });

    assert_not_frozen(env);

    for (size_t i = 0; i < m_vector.size(); ++i) {
        auto &item = (*this)[i];
        m_vector.at(i) = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
    }
    return this;
}

ValuePtr ArrayValue::fill(Env *env, ValuePtr obj, ValuePtr start_obj, ValuePtr length_obj, Block *block) {
    assert_not_frozen(env);

    if (block && length_obj) {
        env->raise("ArgumentError", "wrong number of arguments (given 3, expected 0..2)");
    }

    if (!obj && !block) {
        env->raise("ArgumentError", "wrong number of arguments (given 0, expected 1..3)");
    }

    if (!length_obj && block) {
        length_obj = start_obj;
        start_obj = obj;
    }

    auto to_int = SymbolValue::intern("to_int");
    nat_int_t start = 0;
    nat_int_t max = size();

    if (start_obj && !start_obj->is_nil()) {
        if (!length_obj && start_obj->is_range()) {
            ValuePtr begin = start_obj->as_range()->begin();
            if (!begin.is_integer() && begin->respond_to(env, to_int)) {
                begin = begin->send(env, to_int);
            }
            begin->assert_type(env, Type::Integer, "Integer");
            start = begin->as_integer()->to_nat_int_t();

            if (start < 0)
                start += size();
            if (start < 0)
                env->raise("RangeError", "{} out of range", start_obj->inspect_str(env)->c_str());

            ValuePtr end = start_obj->as_range()->end();
            if (!end.is_integer() && end->respond_to(env, to_int)) {
                end = end->send(env, to_int);
            }
            end->assert_type(env, Type::Integer, "Integer");
            max = end->as_integer()->to_nat_int_t();

            if (max < 0)
                max += size();
            if (max != 0 && !start_obj->as_range()->exclude_end())
                ++max;
        } else {
            if (!start_obj.is_integer() && start_obj->respond_to(env, to_int)) {
                start_obj = start_obj->send(env, to_int);
            }
            start_obj->assert_type(env, Type::Integer, "Integer");
            start = start_obj->as_integer()->to_nat_int_t();

            if (start < 0)
                start += size();
            if (start < 0)
                start = 0;

            if (length_obj && !length_obj->is_nil()) {
                if (!length_obj.is_integer() && length_obj->respond_to(env, to_int)) {
                    length_obj = length_obj->send(env, to_int);
                }
                length_obj->assert_type(env, Type::Integer, "Integer");
                auto length = length_obj->as_integer()->to_nat_int_t();

                if (length <= 0)
                    return this;

                max = start + length;
            }
        }
    }

    if (start >= max)
        return this;

    if (max > (nat_int_t)size())
        expand_with_nil(env, max);

    for (size_t i = start; i < (size_t)max; ++i) {
        if (block) {
            ValuePtr args[1] = { ValuePtr::integer(i) };
            m_vector[i] = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, args, nullptr);
        } else
            m_vector[i] = obj;
    }
    return this;
}

ValuePtr ArrayValue::first(Env *env, ValuePtr n) {
    auto has_count = n != nullptr;

    if (!has_count) {
        if (is_empty())
            return NilValue::the();

        return (*this)[0];
    }

    ArrayValue *array = new ArrayValue();

    auto to_int = SymbolValue::intern("to_int");
    if (!n.is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

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

ValuePtr ArrayValue::delete_at(Env *env, ValuePtr n) {
    this->assert_not_frozen(env);

    auto to_int = SymbolValue::intern("to_int");
    if (!n->is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    if (!n->is_integer())
        env->raise("TypeError", "no implicit conversion of {} into Integer", n->klass()->class_name_or_blank());

    nat_int_t nat_resolved_index = _resolve_index(n->as_integer()->to_nat_int_t());

    if (nat_resolved_index < 0)
        return NilValue::the();

    auto resolved_index = static_cast<size_t>(nat_resolved_index);
    auto value = (*this)[resolved_index];
    m_vector.remove(resolved_index);
    return value;
}

ValuePtr ArrayValue::delete_if(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("delete_if") });

    this->assert_not_frozen(env);

    Vector<size_t> marked_indexes;

    for (size_t i = 0; i < size(); ++i) {
        auto item = (*this)[i];
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy()) {
            marked_indexes.push(i);
        }
    }

    while (!marked_indexes.is_empty()) {
        m_vector.remove(marked_indexes.pop());
    }

    return this;
}

ValuePtr ArrayValue::delete_item(Env *env, ValuePtr target, Block *block) {
    ValuePtr deleted_item = NilValue::the();

    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];
        if (item->neq(env, target))
            continue;

        if (deleted_item->is_nil()) {
            // frozen assertion only happens if any item needs to in fact be deleted
            this->assert_not_frozen(env);
            deleted_item = item;
        }

        m_vector.remove(i - 1);
    }

    if (deleted_item->is_nil() && block) {
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    }

    return deleted_item;
}

ValuePtr ArrayValue::difference(Env *env, size_t argc, ValuePtr *args) {
    ValuePtr last = new ArrayValue { *this };

    for (size_t i = 0; i < argc; i++) {
        last = last->as_array()->sub(env, args[i]);
    }

    return last;
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
    auto to_int = SymbolValue::intern("to_int");
    if (!n.is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    n->assert_type(env, Value::Type::Integer, "Integer");

    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "attempt to drop negative size");
        return nullptr;
    }

    ArrayValue *array = new ArrayValue();
    array->m_klass = klass();
    for (size_t k = n_value; k < size(); ++k) {
        array->push((*this)[k]);
    }

    return array;
}

ValuePtr ArrayValue::drop_while(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("drop_while") });

    ArrayValue *array = new ArrayValue {};
    array->m_klass = klass();

    size_t i = 0;
    while (i < m_vector.size()) {
        ValuePtr args[] = { m_vector.at(i) };
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
        if (result->is_nil() || result->is_false()) {
            break;
        }

        ++i;
    }
    while (i < m_vector.size()) {
        array->push(m_vector.at(i));

        ++i;
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

    auto to_int = SymbolValue::intern("to_int");
    if (!n.is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

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

ValuePtr ArrayValue::include(Env *env, ValuePtr item) {
    if (size() == 0) {
        return FalseValue::the();
    } else {
        for (auto &compare_item : *this) {
            if (compare_item.send(env, SymbolValue::intern("=="), { item })->is_truthy()) {
                return TrueValue::the();
            }
        }
        return FalseValue::the();
    }
}

ValuePtr ArrayValue::index(Env *env, ValuePtr object, Block *block) {
    if (!block && !object) return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("index") });
    return find_index(env, object, block);
}

ValuePtr ArrayValue::shift(Env *env, ValuePtr count) {
    assert_not_frozen(env);
    auto has_count = count != nullptr;
    size_t shift_count = 1;
    ValuePtr result = nullptr;
    if (has_count) {
        auto sym_to_int = SymbolValue::intern("to_int");
        if (count->respond_to(env, sym_to_int)) {
            count = count.send(env, sym_to_int);
        }

        count->assert_type(env, Value::Type::Integer, "Integer");

        auto count_signed = count->as_integer()->to_nat_int_t();

        if (count_signed < 0)
            env->raise("ArgumentError", "negative array size");

        shift_count = count_signed;

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

ValuePtr ArrayValue::keep_if(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("keep_if") });

    select_in_place(env, block);
    return this;
}

ValuePtr ArrayValue::join(Env *env, ValuePtr joiner) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            env->raise("ArgumentError", "recursive array join");
        if (size() == 0) {
            return (ValuePtr) new StringValue {};
        } else if (size() == 1) {
            return (*this)[0].send(env, SymbolValue::intern("to_s"));
        } else {
            if (!joiner || joiner->is_nil())
                joiner = env->global_get(SymbolValue::intern("$,"));
            if (!joiner || joiner->is_nil()) joiner = new StringValue { "" };

            auto to_str = SymbolValue::intern("to_str");
            auto to_s = SymbolValue::intern("to_s");
            if (!joiner->is_string() && joiner->respond_to(env, to_str))
                joiner = joiner->send(env, to_str);

            joiner->assert_type(env, Value::Type::String, "String");
            StringValue *out = new StringValue {};
            for (size_t i = 0; i < size(); i++) {
                ValuePtr item = (*this)[i];
                if (item->is_string())
                    out->append(env, item->as_string());
                else if (item->respond_to(env, to_str))
                    out->append(env, item.send(env, to_str)->as_string());
                else if (item->is_array())
                    out->append(env, item->as_array()->join(env, joiner));
                else if (item->respond_to(env, to_s))
                    out->append(env, item.send(env, to_s)->as_string());
                else {
                    char *buf;
                    asprintf(&buf, "#<%s:%p>", item->klass()->class_name_or_blank()->c_str(), (void *)&item);
                    out->append(env, buf);
                }

                if (i < (size() - 1))
                    out->append(env, joiner->as_string());
            }
            return (ValuePtr)out;
        }
    });
}

ValuePtr ArrayValue::cmp(Env *env, ValuePtr other) {
    auto to_ary = SymbolValue::intern("to_ary");
    if (!other->is_array() && other->respond_to(env, to_ary))
        other = other->send(env, to_ary);

    if (!other->is_array())
        return NilValue::the();

    ArrayValue *other_array = other->as_array();
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return ValuePtr::integer(0);

        for (size_t i = 0; i < size(); i++) {
            if (i >= other_array->size()) {
                return ValuePtr::integer(1);
            }
            ValuePtr item = (*other_array)[i];
            ValuePtr cmp_obj = (*this)[i].send(env, SymbolValue::intern("<=>"), { item });

            if (!cmp_obj->is_integer()) {
                return cmp_obj;
            }

            nat_int_t cmp = cmp_obj->as_integer()->to_nat_int_t();
            if (cmp < 0) return ValuePtr::integer(-1);
            if (cmp > 0) return ValuePtr::integer(1);
        }
        if (other_array->size() > size()) {
            return ValuePtr::integer(-1);
        }
        return ValuePtr::integer(0);
    });
}

ValuePtr ArrayValue::pack(Env *env, ValuePtr directives) {
    if (!directives->is_string() && directives->respond_to(env, SymbolValue::intern("to_str")))
        directives = directives->send(env, SymbolValue::intern("to_str"));

    directives->assert_type(env, Value::Type::String, "String");
    auto directives_string = directives->as_string()->to_low_level_string();

    if (directives_string->is_empty())
        return new StringValue;

    auto packed = ArrayPacker::Packer { this, directives_string }.pack(env);
    return new StringValue { packed, Encoding::ASCII_8BIT };
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

ValuePtr ArrayValue::pop(Env *env, ValuePtr count) {
    this->assert_not_frozen(env);

    if (count) {
        auto to_int = SymbolValue::intern("to_int");

        if (!count->is_integer() && count->respond_to(env, to_int))
            count = count.send(env, to_int);

        count->assert_type(env, ValueType::Integer, "Integer");
        auto c = count->as_integer()->to_nat_int_t();

        if (c < 0)
            env->raise("ArgumentError", "negative array size");

        if (c > (nat_int_t)size())
            c = (nat_int_t)size();

        auto pops = new ArrayValue();
        for (nat_int_t i = 0; i < c; ++i)
            pops->m_vector.push_front(m_vector.pop());

        return pops;
    }
    if (size() == 0) return NilValue::the();
    ValuePtr val = m_vector.pop();
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

    ArrayValue *copy = new ArrayValue(*this);
    copy->select_in_place(env, block);
    return copy;
}

ValuePtr ArrayValue::select_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("select!") });

    assert_not_frozen(env);

    bool changed { false };

    ArrayValue new_array;

    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy())
            new_array.push(item);
        else
            changed = true;
    }

    *this = std::move(new_array);

    if (changed)
        return this;
    return NilValue::the();
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

ValuePtr ArrayValue::max(Env *env, ValuePtr count, Block *block) {
    if (m_vector.size() == 0)
        return NilValue::the();

    auto to_int = SymbolValue::intern("to_int");
    auto is_more = [&](ValuePtr item, ValuePtr min) -> bool {
        ValuePtr block_args[] = { item, min };
        ValuePtr compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, SymbolValue::intern("<=>"), { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->class_name_or_blank(),
                min->klass()->class_name_or_blank());

        if (!compare->is_integer() && compare->respond_to(env, to_int))
            compare = compare.send(env, to_int);

        return compare->as_integer()->to_nat_int_t() > 0;
    };

    auto has_implicit_count = !count || count->is_nil();
    size_t c;

    if (has_implicit_count) {
        c = 1;
    } else {
        if (!count->is_integer() && count->respond_to(env, to_int))
            count = count.send(env, to_int);

        auto c_nat_int = count->as_integer()->to_nat_int_t();
        if (c_nat_int < 0)
            env->raise("ArgumentError", "negative size ({})", c_nat_int);

        c = static_cast<size_t>(c_nat_int);
    }

    Vector<ValuePtr> maxes {};

    for (auto item : *this) {
        for (size_t i = 0; i < maxes.size() && i < c; ++i) {
            if (is_more(item, maxes[i])) {
                maxes.insert(i, item);
                goto next_item;
            }
        }
        maxes.push(item);
    next_item:
        continue;
    }
    if (has_implicit_count)
        return maxes[0];
    return new ArrayValue { maxes.slice(0, c) };
}

ValuePtr ArrayValue::min(Env *env, ValuePtr count, Block *block) {
    if (m_vector.size() == 0)
        return NilValue::the();

    auto to_int = SymbolValue::intern("to_int");
    auto is_less = [&](ValuePtr item, ValuePtr min) -> bool {
        ValuePtr block_args[] = { item, min };
        ValuePtr compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, SymbolValue::intern("<=>"), { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->class_name_or_blank(),
                min->klass()->class_name_or_blank());

        if (!compare->is_integer() && compare->respond_to(env, to_int))
            compare = compare.send(env, to_int);

        return compare->as_integer()->to_nat_int_t() < 0;
    };

    auto has_implicit_count = !count || count->is_nil();
    size_t c;

    if (has_implicit_count) {
        c = 1;
    } else {
        if (!count->is_integer() && count->respond_to(env, to_int))
            count = count.send(env, to_int);

        auto c_nat_int = count->as_integer()->to_nat_int_t();
        if (c_nat_int < 0)
            env->raise("ArgumentError", "negative size ({})", c_nat_int);

        c = static_cast<size_t>(c_nat_int);
    }

    Vector<ValuePtr> mins {};

    for (auto item : *this) {
        for (size_t i = 0; i < mins.size() && i < c; ++i) {
            if (is_less(item, mins[i])) {
                mins.insert(i, item);
                goto next_item;
            }
        }
        mins.push(item);
    next_item:
        continue;
    }
    if (has_implicit_count)
        return mins[0];
    return new ArrayValue { mins.slice(0, c) };
}

ValuePtr ArrayValue::minmax(Env *env, Block *block) {
    if (m_vector.size() == 0)
        return new ArrayValue { NilValue::the(), NilValue::the() };

    auto to_int = SymbolValue::intern("to_int");
    auto compare = [&](ValuePtr item, ValuePtr min) -> nat_int_t {
        ValuePtr block_args[] = { item, min };
        ValuePtr compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, SymbolValue::intern("<=>"), { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->class_name_or_blank(),
                min->klass()->class_name_or_blank());

        if (!compare->is_integer() && compare->respond_to(env, to_int))
            compare = compare.send(env, to_int);

        return compare->as_integer()->to_nat_int_t();
    };

    ValuePtr max;
    ValuePtr min;

    for (auto item : *this) {
        if (max == nullptr || compare(item, max) > 0)
            max = item;
        if (min == nullptr || compare(item, min) < 0)
            min = item;
    }
    return new ArrayValue { min, max };
}

ValuePtr ArrayValue::multiply(Env *env, ValuePtr factor) {
    auto to_str = SymbolValue::intern("to_str");

    if (!factor->is_string() && factor->respond_to(env, to_str))
        factor = factor.send(env, to_str);

    if (factor->is_string()) {
        return join(env, factor);
    }

    auto to_int = SymbolValue::intern("to_int");

    if (!factor->is_integer() && factor->respond_to(env, to_int))
        factor = factor.send(env, to_int);

    if (factor->is_integer()) {
        auto times = factor->as_integer()->to_nat_int_t();

        if (times < 0)
            env->raise("ArgumentError", "negative argument");

        auto accumulator = new ArrayValue {};
        accumulator->m_klass = klass();

        for (nat_int_t i = 0; i < times; ++i)
            accumulator->push_splat(env, this);

        return accumulator;
    }

    env->raise("TypeError", "no implicit conversion of {} into Integer", factor->klass()->class_name_or_blank());

    return this;
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

ValuePtr ArrayValue::cycle(Env *env, ValuePtr count, Block *block) {
    // FIXME: delegating to Enumerable#cycle like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#cycle in MRI and it won't affect Array#cycle.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto none_method = Enumerable->find_method(env, SymbolValue::intern("cycle"));
    return none_method->call(env, this, 1, &count, block);
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

        if ((*sub_array)[0].send(env, SymbolValue::intern("=="), { needle })->is_truthy())
            return sub_array;
    }

    return NilValue::the();
}

ValuePtr ArrayValue::bsearch(Env *env, Block *block) {
    if (!block) {
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("bsearch") });
    }
    auto index = bsearch_index(env, block);
    if (index->is_nil())
        return index;

    return (*this)[index->as_integer()->to_nat_int_t()];
}

ValuePtr ArrayValue::bsearch_index(Env *env, Block *block) {
    if (!block) {
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("bsearch_index") });
    }

    nat_int_t left = 0;
    nat_int_t right = m_vector.size() - 1;
    nat_int_t last_index { -1 };

    do {
        nat_int_t i = floor((left + right) / 2);
        auto outcome = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &(*this)[i], nullptr);
        if (!(
                outcome->is_numeric() || outcome->is_nil() || outcome->is_boolean())) {
            env->raise("TypeError", "wrong argument type {} (must be numeric, true, false or nil)", outcome->klass()->class_name_or_blank());
        }

        if (outcome->is_numeric()) {
            auto result = (outcome.is_integer() ? outcome->as_integer()->to_nat_int_t()
                                                : floor(outcome->as_float()->to_double()));
            if (result == 0) {
                last_index = i;
                break;
            }
            if (result < 0)
                --right;
            ++left;
        } else {
            if (outcome->is_true()) {
                last_index = i;
                --right;
                continue;
            }
            if (last_index >= 0) {
                break;
            }
            ++left;
        }
    } while (left < right);

    if (last_index < 0)
        return NilValue::the();
    return new IntegerValue { last_index };
}

ValuePtr ArrayValue::rassoc(Env *env, ValuePtr needle) {
    for (auto &item : *this) {
        if (!item->is_array())
            continue;

        ArrayValue *sub_array = item->as_array();
        if (sub_array->size() < 2)
            continue;

        if (sub_array->at(1)->send(env, SymbolValue::intern("=="), { needle })->is_truthy())
            return sub_array;
    }

    return NilValue::the();
}

ValuePtr ArrayValue::hash(Env *env) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return ValuePtr { NilValue::the() };

        HashBuilder hash {};
        auto hash_method = SymbolValue::intern("hash");
        auto to_int = SymbolValue::intern("to_int");

        for (size_t i = 0; i < size(); ++i) {
            auto item = (*this)[i];
            auto item_hash = item->send(env, hash_method);

            if (item_hash->is_nil())
                continue;

            // this allows us to return the same hash for recursive arrays:
            // a = []; a << a; a.hash == [a].hash # => true
            // a = []; a << a << a; a.hash == [a, a].hash # => true
            if (item->is_array() && size() == item->as_array()->size() && eql(env, item)->is_truthy())
                continue;

            if (!item_hash->is_integer() && item_hash->respond_to(env, to_int))
                item_hash = item_hash->send(env, to_int);

            hash.append(item_hash->as_integer()->to_nat_int_t());
        }

        return ValuePtr::integer(hash.digest());
    });
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
    return intersection(env, 1, &arg);
}

bool ArrayValue::include_eql(Env *env, ValuePtr arg) {
    auto eql = SymbolValue::intern("eql?");
    for (auto &val : *this) {
        if (arg->object_id() == val->object_id() || arg->send(env, eql, { val })->is_truthy())
            return true;
    }
    return false;
}

ValuePtr ArrayValue::intersection(Env *env, size_t argc, ValuePtr *args) {
    auto *result = new ArrayValue { *this };
    result->uniq_in_place(env, nullptr);

    TM::Vector<ArrayValue *> arrays;
    auto to_ary = SymbolValue::intern("to_ary");

    for (size_t i = 0; i < argc; ++i) {
        auto &arg = args[i];
        if (!arg->is_array() && arg->respond_to(env, to_ary)) {
            arg = arg->send(env, to_ary);
        }
        arg->assert_type(env, Type::Array, "Array");

        auto *other_array = arg->as_array();
        if (!other_array->is_empty())
            arrays.push(other_array);
    }

    if (result->is_empty()) return result;
    if (arrays.size() != argc) return new ArrayValue;

    for (size_t i = 0; i < result->size(); ++i) {
        auto &item = result->at(i);
        for (auto *array : arrays) {
            if (!array->include_eql(env, item)) {
                result->m_vector.remove(i--);
                break;
            }
        }
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
    auto *result = new ArrayValue(*this);

    // TODO: we probably want to make | call this instead of this way for optimization
    for (size_t i = 0; i < argc; i++) {
        auto &arg = args[i];
        result = result->union_of(env, arg)->as_array();
    }

    return result;
}

ValuePtr ArrayValue::unshift(Env *env, size_t argc, ValuePtr *args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < argc; i++) {
        m_vector.insert(i, args[i]);
    }
    return this;
}

ValuePtr ArrayValue::reverse(Env *env) {
    ArrayValue *copy = new ArrayValue(*this);
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

    ArrayValue *original = new ArrayValue(*this);

    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        auto original_class_name = arg->klass()->class_name_or_blank();
        auto to_ary = SymbolValue::intern("to_ary");
        if (!arg->is_array() && arg->respond_to(env, to_ary)) {
            arg = arg->send(env, to_ary);
        }

        if (!arg->is_array()) {
            env->raise("TypeError", "no implicit conversion of {} into Array", original_class_name);
            return nullptr;
        }

        concat(*arg->as_array());
    }

    return this;
}

nat_int_t ArrayValue::_resolve_index(nat_int_t nat_index) {
    nat_int_t index { nat_index };

    if (nat_index < 0)
        index += size();

    if (index < 0)
        return -1;

    if (static_cast<size_t>(index) >= size())
        return -1;

    return index;
}

ValuePtr ArrayValue::rindex(Env *env, ValuePtr object, Block *block) {
    if (!block && !object) return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("rindex") });
    return find_index(env, object, block, true);
}

ValuePtr ArrayValue::fetch(Env *env, ValuePtr arg_index, ValuePtr default_value, Block *block) {
    auto to_int = SymbolValue::intern("to_int");
    ValuePtr index_obj = arg_index;

    if (!arg_index.is_integer()) {
        if (arg_index->respond_to(env, to_int)) {
            index_obj = arg_index->send(env, to_int);
            if (!index_obj.is_integer()) {
                auto arg_index_class = arg_index->klass()->inspect_str(env);
                env->raise("TypeError", "can't convert {} to Integer ({}#to_int gives {})", arg_index_class, arg_index_class, index_obj->klass()->inspect_str(env));
            }
        }
    }
    index_obj->assert_type(env, Type::Integer, "Integer");

    ValuePtr value;
    auto index_val = index_obj->as_integer()->to_nat_int_t();
    auto index = _resolve_index(index_val);
    if (index < 0) {
        if (block) {
            if (default_value)
                env->warn("block supersedes default value argument");

            value = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &arg_index, nullptr);
        } else if (default_value) {
            value = default_value;
        } else {
            env->raise("IndexError", "index {} outside of array bounds: {}...{}", index_val, -(nat_int_t)size(), size());
        }
    } else {
        value = m_vector[index];
    }

    return value;
}

ValuePtr ArrayValue::find_index(Env *env, ValuePtr object, Block *block, bool search_reverse) {
    if (object && block) env->warn("given block not used");
    assert(size() <= NAT_INT_MAX);

    auto length = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < length; i++) {
        nat_int_t index = search_reverse ? length - i - 1 : i;
        auto item = m_vector[index];
        if (object) {
            if (item.send(env, SymbolValue::intern("=="), { object })->is_truthy())
                return ValuePtr::integer(index);
        } else {
            ValuePtr args[] = { item };
            auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
            length = static_cast<nat_int_t>(size());
            if (result->is_truthy())
                return ValuePtr::integer(index);
        }
    }
    return NilValue::the();
}

ValuePtr ArrayValue::none(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: delegating to Enumerable#none? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#none? in MRI and it won't affect Array#none?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto none_method = Enumerable->find_method(env, SymbolValue::intern("none?"));
    return none_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::one(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: delegating to Enumerable#one? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#one? in MRI and it won't affect Array#one?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto one_method = Enumerable->find_method(env, SymbolValue::intern("one?"));
    return one_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::rotate(Env *env, ValuePtr val) {
    ArrayValue *copy = new ArrayValue(*this);
    copy->rotate_in_place(env, val);
    return copy;
}

ValuePtr ArrayValue::rotate_in_place(Env *env, ValuePtr val) {
    assert_not_frozen(env);
    nat_int_t count = 1;
    auto to_int = SymbolValue::intern("to_int");
    if (val) {
        if (!val->is_integer() && val->respond_to(env, to_int)) {
            val = val->send(env, to_int);
        } else if (!val->is_integer()) {
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
    this->assert_not_frozen(env);
    auto to_int = SymbolValue::intern("to_int");

    if (size) {
        if (!index_obj->is_integer() && index_obj->respond_to(env, to_int))
            index_obj = index_obj.send(env, to_int);

        index_obj->assert_type(env, ValueType::Integer, "Integer");

        if (!size->is_integer() && size->respond_to(env, to_int))
            size = size.send(env, to_int);

        size->assert_type(env, ValueType::Integer, "Integer");
    }

    if (index_obj->is_integer()) {
        auto start = index_obj.to_nat_int_t();

        if (!size) {
            start = _resolve_index(start);
            if (start < 0 || start >= (nat_int_t)m_vector.size())
                return NilValue::the();

            ValuePtr item = (*this)[start];
            for (size_t i = start; i < this->size() - 1; i++) {
                (*this)[i] = (*this)[i + 1];
            }
            m_vector.pop();
            return item;
        }

        size->assert_type(env, ValueType::Integer, "Integer");

        nat_int_t length = size.to_nat_int_t();

        if (length < 0)
            return NilValue::the();

        if (start > (nat_int_t)m_vector.size())
            return NilValue::the();

        if (start == (nat_int_t)m_vector.size())
            return new ArrayValue {};

        return _slice_in_place(start, _resolve_index(start) + length, true);
    }

    if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();

        if (!begin_obj->is_integer() && begin_obj->respond_to(env, to_int))
            begin_obj = begin_obj.send(env, to_int);

        begin_obj->assert_type(env, ValueType::Integer, "Integer");

        ValuePtr end_obj = range->end();

        if (!end_obj->is_integer() && end_obj->respond_to(env, to_int))
            end_obj = end_obj.send(env, to_int);

        end_obj->assert_type(env, ValueType::Integer, "Integer");

        nat_int_t start = begin_obj.to_nat_int_t();
        nat_int_t end = end_obj.to_nat_int_t();

        return _slice_in_place(start, end, range->exclude_end());
    }

    if (!index_obj->is_integer() && index_obj->respond_to(env, to_int))
        index_obj = index_obj.send(env, to_int);

    index_obj->assert_type(env, ValueType::Integer, "Integer");

    return slice_in_place(env, index_obj, size);
}

ValuePtr ArrayValue::_slice_in_place(nat_int_t start, nat_int_t end, bool exclude_end) {
    if (start > (nat_int_t)m_vector.size())
        return NilValue::the();

    if (start == (nat_int_t)m_vector.size())
        return new ArrayValue {};

    start = _resolve_index(start);

    if (start < 0)
        return NilValue::the();

    if (end < 0)
        end = _resolve_index(end);

    nat_int_t length = end - start + (exclude_end ? 0 : 1);

    if (length < 0)
        length = 0;

    if (length + start > (nat_int_t)this->size())
        length = this->size() - start;

    if (length < 0)
        return NilValue::the();

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

ValuePtr ArrayValue::values_at(Env *env, size_t argc, ValuePtr *args) {
    auto accumulator = new ArrayValue {};
    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];
        auto to_int = SymbolValue::intern("to_int");
        if (!arg->is_integer() && arg->respond_to(env, to_int)) {
            arg = arg.send(env, to_int);
        }

        if (!arg->is_integer())
            env->raise("TypeError", "no implicit conversion of {} into Integer", arg->klass()->class_name_or_blank());

        auto resolved_index = _resolve_index(arg->as_integer()->to_nat_int_t());
        if (resolved_index < 0 || static_cast<size_t>(resolved_index) >= m_vector.size()) {
            accumulator->push(NilValue::the());
            continue;
        }
        accumulator->push((*this)[resolved_index]);
    }

    return accumulator;
}

ValuePtr ArrayValue::zip(Env *env, size_t argc, ValuePtr *args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Enumerable"))->as_module();
    auto zip_method = Enumerable->find_method(env, SymbolValue::intern("zip"));
    return zip_method->call(env, this, argc, args, block);
}
}
