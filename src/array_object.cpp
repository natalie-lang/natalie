#include "natalie.hpp"
#include <algorithm>
#include <math.h>
#include <natalie/array_object.hpp>
#include <natalie/array_packer/packer.hpp>
#include <natalie/string_object.hpp>
#include <natalie/symbol_object.hpp>
#include <random>
#include <tm/hashmap.hpp>
#include <tm/recursion_guard.hpp>

namespace Natalie {

Value ArrayObject::initialize(Env *env, Value size, Value value, Block *block) {
    this->assert_not_frozen(env);

    if (!size) {
        ArrayObject new_array;
        *this = std::move(new_array);
        return this;
    }

    if (!value) {
        auto to_ary = "to_ary"_s;
        if (!size->is_array() && size->respond_to(env, to_ary))
            size = size->send(env, to_ary);

        if (size->is_array()) {
            auto target = *size->as_array();
            *this = std::move(target);
            return this;
        }
    }

    auto to_int = "to_int"_s;
    if (!size->is_integer() && size->respond_to(env, to_int))
        size = size->send(env, to_int);

    size->assert_type(env, Object::Type::Integer, "Integer");

    if (size->as_integer()->is_bignum())
        env->raise("ArgumentError", "array size too big");

    auto s = size->as_integer()->to_nat_int_t();

    if (s < 0)
        env->raise("ArgumentError", "negative argument");

    if (block) {
        if (value)
            env->warn("block supersedes default value argument");

        ArrayObject new_array;
        *this = std::move(new_array);

        for (nat_int_t i = 0; i < s; i++) {
            Value args = new IntegerObject { i };
            push(NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &args, nullptr));
        }

    } else {
        if (!value) value = NilObject::the();
        for (nat_int_t i = 0; i < s; i++) {
            push(value);
        }
    }
    return this;
}

Value ArrayObject::initialize_copy(Env *env, Value other) {
    assert_not_frozen(env);

    ArrayObject *other_array = other->to_ary(env);

    clear(env);
    for (auto &item : *other_array) {
        m_vector.push(item);
    }
    return this;
}

Value ArrayObject::first() {
    if (m_vector.is_empty())
        return NilObject::the();
    return m_vector[0];
}

Value ArrayObject::last() {
    if (m_vector.is_empty())
        return NilObject::the();
    return m_vector[m_vector.size() - 1];
}

Value ArrayObject::inspect(Env *env) {
    RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return new StringObject { "[...]" };

        StringObject *out = new StringObject { "[" };
        for (size_t i = 0; i < size(); i++) {
            Value obj = (*this)[i];

            auto inspected_repr = obj.send(env, "inspect"_s);
            SymbolObject *to_s = "to_s"_s;

            if (!inspected_repr->is_string()) {
                if (inspected_repr->respond_to(env, to_s)) {
                    inspected_repr = obj.send(env, to_s);
                }
            }

            if (inspected_repr->is_string())
                out->append(env, inspected_repr->as_string());
            else
                out->append_sprintf("#<%s:%#x>", inspected_repr->klass()->inspect_str()->c_str(), static_cast<uintptr_t>(inspected_repr));

            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(']');
        return out;
    });
}

Value ArrayObject::ltlt(Env *env, Value arg) {
    this->assert_not_frozen(env);
    push(arg);
    return this;
}

Value ArrayObject::add(Env *env, Value other) {
    ArrayObject *other_array = other->to_ary(env);

    ArrayObject *new_array = new ArrayObject { *this };
    new_array->concat(*other_array);
    return new_array;
}

Value ArrayObject::sub(Env *env, Value other) {
    ArrayObject *other_array = other->to_ary(env);

    ArrayObject *new_array = new ArrayObject {};
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other_array) {
            if ((item.send(env, "eql?"_s, { compare_item })->is_truthy() && item.send(env, "hash"_s).send(env, "eql?"_s, { compare_item.send(env, "hash"_s) }))
                || item.send(env, "=="_s, { compare_item })->is_truthy()) {
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

Value ArrayObject::sum(Env *env, size_t argc, Value *args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto sum_method = Enumerable->find_method(env, "sum"_s);
    return sum_method->call(env, this, argc, args, block);
}

Value ArrayObject::ref(Env *env, Value index_obj, Value size) {
    if (!size) {
        if (!index_obj->is_integer() && index_obj->respond_to(env, "to_int"_s))
            index_obj = index_obj->send(env, "to_int"_s);

        if (index_obj->is_integer()) {
            index_obj->as_integer()->assert_fixnum(env);

            auto index = _resolve_index(index_obj->as_integer()->to_nat_int_t());
            if (index < 0 || index >= (nat_int_t)m_vector.size())
                return NilObject::the();
            return m_vector[index];
        }
    }

    ArrayObject *copy = new ArrayObject { *this };
    return copy->slice_in_place(env, index_obj, size);
}

Value ArrayObject::refeq(Env *env, Value index_obj, Value size, Value val) {
    this->assert_not_frozen(env);
    auto to_int = "to_int"_s;
    nat_int_t start, width;
    if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();
        Value begin_obj = range->begin();
        Value end_obj = range->end();

        // Ignore "size"
        val = size;

        if (begin_obj->is_nil()) {
            start = 0;
        } else {
            if (!begin_obj->is_integer() && begin_obj->respond_to(env, to_int))
                begin_obj = begin_obj->send(env, to_int);
            begin_obj->assert_type(env, Object::Type::Integer, "Integer");

            start = begin_obj->as_integer()->to_nat_int_t();
            if (start < 0) {
                if ((size_t)(-start) > this->size()) {
                    env->raise("RangeError", "{} out of range", range->inspect_str(env));
                    return nullptr;
                }
                start = this->size() + start;
            }
        }

        nat_int_t end;
        if (end_obj->is_nil()) {
            end = this->size();
        } else {
            if (!end_obj->is_integer() && end_obj->respond_to(env, to_int))
                end_obj = end_obj->send(env, to_int);
            end_obj->assert_type(env, Object::Type::Integer, "Integer");

            end = end_obj->as_integer()->to_nat_int_t();
            if (end < 0) {
                end = this->size() + end;
            }
        }

        width = end - start + (range->exclude_end() ? 0 : 1);
        if (width < 0) {
            width = 0;
        }
        if (width + start > (nat_int_t)this->size()) {
            width = this->size() - start;
        }
    } else {
        if (!index_obj->is_integer() && index_obj->respond_to(env, to_int))
            index_obj = index_obj->send(env, to_int);
        index_obj->assert_type(env, ObjectType::Integer, "Integer");

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

        if (!size->is_integer() && size->respond_to(env, to_int))
            size = size->send(env, to_int);

        size->assert_type(env, Object::Type::Integer, "Integer");
        width = size->as_integer()->to_nat_int_t();
        if (width < 0) {
            env->raise("IndexError", "negative length ({})", width);
            return nullptr;
        }
    }

    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayObject *new_ary = new ArrayObject {};

    // stuff before the new entry/entries
    for (size_t i = 0; i < (size_t)start; i++) {
        if (i >= this->size()) break;
        new_ary->push((*this)[i]);
    }

    // extra nils if needed
    new_ary->expand_with_nil(env, start);

    // the new entry/entries
    auto to_ary = "to_ary"_s;
    if (val->is_array() || val->respond_to(env, to_ary)) {
        if (!val->is_array())
            val = val.send(env, to_ary);

        val->assert_type(env, Object::Type::Array, "Array");

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

Value ArrayObject::any(Env *env, size_t argc, Value *args, Block *block) {
    // FIXME: delegating to Enumerable#any? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#any? in MRI and it won't affect Array#any?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto any_method = Enumerable->find_method(env, "any?"_s);
    return any_method->call(env, this, argc, args, block);
}

bool ArrayObject::eq(Env *env, Value other) {
    TM::PairedRecursionGuard guard { this, other.object() };

    Value result = guard.run([&](bool is_recursive) -> Value { // don't return bool in recursion guard
        if (other == this)
            return TrueObject::the();

        SymbolObject *equality = "=="_s;
        if (!other->is_array()
            && other->send(env, "respond_to?"_s, { "to_ary"_s })->is_true())
            return other->send(env, equality, { this });

        if (!other->is_array()) {
            return FalseObject::the();
        }

        auto other_array = other->as_array();
        if (size() != other_array->size())
            return FalseObject::the();

        if (is_recursive)
            // since == is an & of all the == of each value, this will just leave the expression uneffected
            return TrueObject::the();

        SymbolObject *object_id = "object_id"_s;
        for (size_t i = 0; i < size(); ++i) {
            Value this_item = (*this)[i];
            Value item = (*other_array)[i];

            if (this_item->respond_to(env, object_id) && item->respond_to(env, object_id)) {
                Value same_object_id = this_item
                                           ->send(env, object_id)
                                           ->send(env, equality, { item->send(env, object_id) });

                // This allows us to check NAN equality and other potentially similar constants
                if (same_object_id->is_true())
                    continue;
            }

            Value result = this_item.send(env, equality, 1, &item, nullptr);
            if (result->is_false())
                return result;
        }

        return TrueObject::the();
    });

    return result->is_true();
}

bool ArrayObject::eql(Env *env, Value other) {
    TM::PairedRecursionGuard guard { this, other.object() };

    Value result = guard.run([&](bool is_recursive) -> Value { // don't return bool in recursion guard
        if (other == this)
            return TrueObject::the();
        if (!other->is_array())
            return FalseObject::the();

        if (!other->is_array()) {
            return FalseObject::the();
        }

        auto other_array = other->as_array();
        if (size() != other_array->size())
            return FalseObject::the();

        if (is_recursive)
            // since eql is an & of all the eql of each value, this will just leave the expression uneffected
            return TrueObject::the();

        for (size_t i = 0; i < size(); ++i) {
            Value item = (*other_array)[i];
            Value result = (*this)[i].send(env, "eql?"_s, 1, &item, nullptr);
            if (result->is_false())
                return result;
        }

        return TrueObject::the();
    });

    return result->is_true();
}

Value ArrayObject::each(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "each"_s });

    for (size_t i = 0; i < size(); ++i) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &(*this)[i], nullptr);
    }
    return this;
}

Value ArrayObject::each_index(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "each_index"_s });

    nat_int_t size_nat_int_t = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < size_nat_int_t; i++) {
        Value args = new IntegerObject { i };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &args, nullptr);
    }
    return this;
}

Value ArrayObject::map(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "map"_s });

    ArrayObject *copy = new ArrayObject { *this };
    copy->map_in_place(env, block);
    return copy;
}

Value ArrayObject::map_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "map!"_s });

    assert_not_frozen(env);

    for (size_t i = 0; i < m_vector.size(); ++i) {
        auto &item = (*this)[i];
        m_vector.at(i) = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
    }
    return this;
}

Value ArrayObject::fill(Env *env, Value obj, Value start_obj, Value length_obj, Block *block) {
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

    auto to_int = "to_int"_s;
    nat_int_t start = 0;
    nat_int_t max = size();

    if (start_obj && !start_obj->is_nil()) {
        if (!length_obj && start_obj->is_range()) {
            Value begin = start_obj->as_range()->begin();
            if (!begin->is_nil()) {
                if (!begin->is_integer() && begin->respond_to(env, to_int)) {
                    begin = begin->send(env, to_int);
                }
                begin->assert_type(env, Type::Integer, "Integer");
                start = begin->as_integer()->to_nat_int_t();

                if (start < 0)
                    start += size();
                if (start < 0)
                    env->raise("RangeError", "{} out of range", start_obj->inspect_str(env)->c_str());
            }

            auto end = start_obj->as_range()->end();

            if (!end->is_nil()) {
                if (!end->is_integer() && end->respond_to(env, to_int)) {
                    end = end->send(env, to_int);
                }
                end->assert_type(env, Type::Integer, "Integer");
                max = end->as_integer()->to_nat_int_t();

                if (max < 0)
                    max += size();
                if (max != 0 && !start_obj->as_range()->exclude_end())
                    ++max;
            }
        } else {
            if (!start_obj->is_integer() && start_obj->respond_to(env, to_int)) {
                start_obj = start_obj->send(env, to_int);
            }
            start_obj->assert_type(env, Type::Integer, "Integer");
            start = start_obj->as_integer()->to_nat_int_t();

            if (start < 0)
                start += size();
            if (start < 0)
                start = 0;

            if (length_obj && !length_obj->is_nil()) {
                if (!length_obj->is_integer() && length_obj->respond_to(env, to_int)) {
                    length_obj = length_obj->send(env, to_int);
                }
                length_obj->assert_type(env, Type::Integer, "Integer");
                length_obj->as_integer()->assert_fixnum(env);
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
            Value args[1] = { Value::integer(i) };
            m_vector[i] = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, args, nullptr);
        } else
            m_vector[i] = obj;
    }
    return this;
}

Value ArrayObject::first(Env *env, Value n) {
    auto has_count = n != nullptr;

    if (!has_count) {
        if (is_empty())
            return NilObject::the();

        return (*this)[0];
    }

    auto to_int = "to_int"_s;
    if (!n->is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    n->assert_type(env, Object::Type::Integer, "Integer");

    n->as_integer()->assert_fixnum(env);

    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "negative array size");
        return nullptr;
    }

    size_t end = std::min(size(), (size_t)n_value);
    ArrayObject *array = new ArrayObject { end };

    for (size_t k = 0; k < end; ++k) {
        array->push((*this)[k]);
    }

    return array;
}

Value ArrayObject::flatten(Env *env, Value depth) {
    ArrayObject *copy = new ArrayObject { *this };
    copy->flatten_in_place(env, depth);
    return copy;
}

Value ArrayObject::flatten_in_place(Env *env, Value depth) {
    this->assert_not_frozen(env);

    bool changed { false };

    auto has_depth = depth != nullptr;
    if (has_depth) {
        auto sym_to_i = "to_i"_s;
        auto sym_to_int = "to_int"_s;
        if (!depth->is_integer()) {
            if (depth->respond_to(env, sym_to_i)) {
                depth = depth.send(env, sym_to_i);
            } else if (depth->respond_to(env, sym_to_int)) {
                depth = depth.send(env, sym_to_int);
            } else {
                env->raise("TypeError", "no implicit conversion of {} into Integer", depth->klass()->inspect_str());
                return nullptr;
            }
        }

        depth->assert_type(env, Object::Type::Integer, "Integer");
        nat_int_t depth_value = depth->as_integer()->to_nat_int_t();

        changed = _flatten_in_place(env, depth_value);
    } else {
        changed = _flatten_in_place(env, -1);
    }

    if (changed)
        return this;

    return NilObject::the();
}

bool ArrayObject::_flatten_in_place(Env *env, nat_int_t depth, Hashmap<ArrayObject *> visited) {
    bool changed { false };

    if (visited.is_empty())
        visited.set(this);

    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];

        if (depth == 0 || item == nullptr) {
            continue;
        }

        if (!item->is_array()) {
            Value new_item = try_convert(env, item);

            if (!new_item->is_nil()) {
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
            ArrayObject *copy = new ArrayObject { *array_item };

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

Value ArrayObject::delete_at(Env *env, Value n) {
    this->assert_not_frozen(env);

    auto to_int = "to_int"_s;
    if (!n->is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    if (!n->is_integer())
        env->raise("TypeError", "no implicit conversion of {} into Integer", n->klass()->inspect_str());

    nat_int_t nat_resolved_index = _resolve_index(n->as_integer()->to_nat_int_t());

    if (nat_resolved_index < 0)
        return NilObject::the();

    auto resolved_index = static_cast<size_t>(nat_resolved_index);
    auto value = (*this)[resolved_index];
    m_vector.remove(resolved_index);
    return value;
}

Value ArrayObject::delete_if(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "delete_if"_s });

    this->assert_not_frozen(env);

    Vector<size_t> marked_indexes;

    for (size_t i = 0; i < size(); ++i) {
        auto item = (*this)[i];
        Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy()) {
            marked_indexes.push(i);
        }
    }

    while (!marked_indexes.is_empty()) {
        m_vector.remove(marked_indexes.pop());
    }

    return this;
}

Value ArrayObject::delete_item(Env *env, Value target, Block *block) {
    Value deleted_item = NilObject::the();

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

Value ArrayObject::difference(Env *env, size_t argc, Value *args) {
    Value last = new ArrayObject { *this };

    for (size_t i = 0; i < argc; i++) {
        last = last->as_array()->sub(env, args[i]);
    }

    return last;
}

Value ArrayObject::dig(Env *env, size_t argc, Value *args) {
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

Value ArrayObject::drop(Env *env, Value n) {
    auto to_int = "to_int"_s;
    if (!n->is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    n->assert_type(env, Object::Type::Integer, "Integer");

    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "attempt to drop negative size");
        return nullptr;
    }

    ArrayObject *array = new ArrayObject { (size_t)std::max((nat_int_t)size() - n_value, (nat_int_t)0) };
    array->m_klass = klass();
    for (size_t k = n_value; k < size(); ++k) {
        array->push((*this)[k]);
    }

    return array;
}

Value ArrayObject::drop_while(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "drop_while"_s });

    ArrayObject *array = new ArrayObject {};
    array->m_klass = klass();

    size_t i = 0;
    while (i < m_vector.size()) {
        Value args[] = { m_vector.at(i) };
        Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
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

Value ArrayObject::last(Env *env, Value n) {
    auto has_count = n != nullptr;

    if (!has_count) {
        if (is_empty())
            return NilObject::the();

        return (*this)[size() - 1];
    }

    auto to_int = "to_int"_s;
    if (!n->is_integer() && n->respond_to(env, to_int))
        n = n->send(env, to_int);

    n->assert_type(env, Object::Type::Integer, "Integer");
    nat_int_t n_value = n->as_integer()->to_nat_int_t();

    if (n_value < 0) {
        env->raise("ArgumentError", "negative array size");
        return nullptr;
    }

    assert(size() <= NAT_INT_MAX);
    nat_int_t signed_size = static_cast<nat_int_t>(size());
    size_t start = std::max(static_cast<nat_int_t>(0), signed_size - n_value);
    ArrayObject *array = new ArrayObject(size() - start);

    for (size_t k = start; k < size(); ++k) {
        array->push((*this)[k]);
    }

    return array;
}

bool ArrayObject::include(Env *env, Value item) {
    if (size() == 0) {
        return false;
    } else {
        for (auto &compare_item : *this) {
            if (compare_item.send(env, "=="_s, { item })->is_truthy()) {
                return true;
            }
        }
        return false;
    }
}

Value ArrayObject::index(Env *env, Value object, Block *block) {
    if (!block && !object) return send(env, "enum_for"_s, { "index"_s });
    return find_index(env, object, block);
}

Value ArrayObject::shift(Env *env, Value count) {
    assert_not_frozen(env);
    auto has_count = count != nullptr;
    size_t shift_count = 1;
    Value result = nullptr;
    if (has_count) {
        auto sym_to_int = "to_int"_s;
        if (count->respond_to(env, sym_to_int)) {
            count = count.send(env, sym_to_int);
        }

        count->assert_type(env, Object::Type::Integer, "Integer");

        auto count_signed = count->as_integer()->to_nat_int_t();

        if (count_signed < 0)
            env->raise("ArgumentError", "negative array size");

        shift_count = count_signed;

        if (shift_count == 0) {
            return new ArrayObject {};
        }
        result = new ArrayObject { m_vector.slice(0, shift_count) };
    } else {
        result = m_vector[0];
    }

    auto shifted = m_vector.slice(shift_count);
    m_vector = std::move(shifted);
    return result;
}

Value ArrayObject::sort(Env *env, Block *block) {
    ArrayObject *copy = new ArrayObject { *this };
    copy->sort_in_place(env, block);
    return copy;
}

Value ArrayObject::keep_if(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "keep_if"_s });

    select_in_place(env, block);
    return this;
}

Value ArrayObject::join(Env *env, Value joiner) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            env->raise("ArgumentError", "recursive array join");
        if (size() == 0) {
            return (Value) new StringObject {};
        } else if (size() == 1) {
            return (*this)[0].send(env, "to_s"_s);
        } else {
            if (!joiner || joiner->is_nil())
                joiner = env->global_get("$,"_s);
            if (!joiner || joiner->is_nil()) joiner = new StringObject { "" };

            auto to_str = "to_str"_s;
            auto to_s = "to_s"_s;
            if (!joiner->is_string() && joiner->respond_to(env, to_str))
                joiner = joiner->send(env, to_str);

            joiner->assert_type(env, Object::Type::String, "String");
            StringObject *out = new StringObject {};
            for (size_t i = 0; i < size(); i++) {
                Value item = (*this)[i];
                if (item->is_string())
                    out->append(env, item->as_string());
                else if (item->respond_to(env, to_str))
                    out->append(env, item.send(env, to_str)->as_string());
                else if (item->is_array())
                    out->append(env, item->as_array()->join(env, joiner));
                else if (item->respond_to(env, to_s))
                    out->append(env, item.send(env, to_s)->as_string());
                else
                    out->append(env, String::format("#<{}:{}>", item->klass()->inspect_str()->c_str(), static_cast<size_t>(item)));

                if (i < (size() - 1))
                    out->append(env, joiner->as_string());
            }
            return (Value)out;
        }
    });
}

Value ArrayObject::cmp(Env *env, Value other) {
    Value other_converted = try_convert(env, other);

    if (other_converted->is_nil()) {
        return other_converted;
    }

    ArrayObject *other_array = other_converted->as_array();
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return Value::integer(0);

        for (size_t i = 0; i < size(); i++) {
            if (i >= other_array->size()) {
                return Value::integer(1);
            }
            Value item = (*other_array)[i];
            Value cmp_obj = (*this)[i].send(env, "<=>"_s, { item });

            if (!cmp_obj->is_integer()) {
                return cmp_obj;
            }

            nat_int_t cmp = cmp_obj->as_integer()->to_nat_int_t();
            if (cmp < 0) return Value::integer(-1);
            if (cmp > 0) return Value::integer(1);
        }
        if (other_array->size() > size()) {
            return Value::integer(-1);
        }
        return Value::integer(0);
    });
}

Value ArrayObject::pack(Env *env, Value directives) {
    if (!directives->is_string() && directives->respond_to(env, "to_str"_s))
        directives = directives->send(env, "to_str"_s);

    directives->assert_type(env, Object::Type::String, "String");
    auto directives_string = directives->as_string()->to_low_level_string();

    if (directives_string->is_empty())
        return new StringObject;

    return ArrayPacker::Packer { this, directives_string }.pack(env);
}

Value ArrayObject::push(Env *env, size_t argc, Value *args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < argc; i++) {
        push(args[i]);
    }
    return this;
}

void ArrayObject::push_splat(Env *env, Value val) {
    if (!val->is_array() && val->respond_to(env, "to_a"_s)) {
        val = val.send(env, "to_a"_s);
    }
    if (val->is_array()) {
        m_vector.set_capacity(m_vector.capacity() + val->as_array()->size());
        for (Value v : *val->as_array()) {
            push(*v);
        }
    } else {
        push(*val);
    }
}

Value ArrayObject::pop(Env *env, Value count) {
    this->assert_not_frozen(env);

    if (count) {
        auto to_int = "to_int"_s;

        if (!count->is_integer() && count->respond_to(env, to_int))
            count = count.send(env, to_int);

        count->assert_type(env, ObjectType::Integer, "Integer");
        auto c = count->as_integer()->to_nat_int_t();

        if (c < 0)
            env->raise("ArgumentError", "negative array size");

        if (c > (nat_int_t)size())
            c = (nat_int_t)size();

        auto pops = new ArrayObject { (size_t)c };
        for (nat_int_t i = 0; i < c; ++i)
            pops->m_vector.push_front(m_vector.pop());

        return pops;
    }
    if (size() == 0) return NilObject::the();
    Value val = m_vector.pop();
    return val;
}

void ArrayObject::expand_with_nil(Env *env, size_t total) {
    for (size_t i = size(); i < total; i++) {
        push(*NilObject::the());
    }
}

bool array_sort_compare(Env *env, Value a, Value b, Block *block) {
    if (block) {
        Value args[2] = { a, b };
        Value compare = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 2, args, nullptr);

        if (compare->respond_to(env, "<"_s)) {
            Value zero = Value::integer(0);
            return compare.send(env, "<"_s, { zero })->is_truthy();
        } else {
            env->raise("ArgumentError", "comparison of {} with 0 failed", compare->klass()->inspect_str());
        }
    } else {
        Value compare = a.send(env, "<=>"_s, { b });
        if (compare->is_integer()) {
            return compare->as_integer()->to_nat_int_t() < 0;
        }
        // TODO: Ruby sometimes prints b as the value (for example for integers) and sometimes as class
        env->raise("ArgumentError", "comparison of {} with {} failed", a->klass()->inspect_str(), b->klass()->inspect_str());
    }
}

Value ArrayObject::sort_in_place(Env *env, Block *block) {
    this->assert_not_frozen(env);

    m_vector.sort([env, block](Value a, Value b) {
        return array_sort_compare(env, a, b, block);
    });

    return this;
}

bool array_sort_by_compare(Env *env, Value a, Value b, Block *block) {
    Value a_res = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &a, nullptr);
    Value b_res = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &b, nullptr);

    Value compare = a_res.send(env, "<=>"_s, { b_res });
    if (compare->is_integer()) {
        return compare->as_integer()->to_nat_int_t() < 0;
    }
    env->raise("ArgumentError", "comparison of {} with {} failed", a_res->klass()->inspect_str(), b_res->klass()->inspect_str());
}

Value ArrayObject::sort_by_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "sort_by!"_s });

    this->assert_not_frozen(env);

    m_vector.sort([env, block](Value a, Value b) {
        return array_sort_by_compare(env, a, b, block);
    });

    return this;
}

Value ArrayObject::select(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "select"_s });

    ArrayObject *copy = new ArrayObject(*this);
    copy->select_in_place(env, block);
    return copy;
}

Value ArrayObject::select_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "select!"_s });

    assert_not_frozen(env);

    bool changed = select_in_place([env, block](Value &item) -> bool {
        Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        return result->is_truthy();
    });

    if (changed)
        return this;
    return NilObject::the();
}

bool ArrayObject::select_in_place(std::function<bool(Value &)> predicate) {
    bool changed { false };

    ArrayObject new_array;

    for (auto &item : *this) {
        if (predicate(item))
            new_array.push(item);
        else
            changed = true;
    }

    *this = std::move(new_array);

    return changed;
}

Value ArrayObject::reject(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "reject"_s });

    ArrayObject *copy = new ArrayObject(*this);
    copy->reject_in_place(env, block);
    return copy;
}

Value ArrayObject::reject_in_place(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "reject!"_s });

    assert_not_frozen(env);

    bool changed { false };

    ArrayObject new_array;

    for (auto it = begin(); it != end(); it++) {
        auto item = *it;

        try {
            Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
            if (result->is_falsey())
                new_array.push(item);
            else
                changed = true;
        } catch (ExceptionObject *exception) {
            new_array.push(item);
            for (it++; it != end(); it++) {
                new_array.push(*it);
            }

            *this = std::move(new_array);

            throw;
        }
    }

    *this = std::move(new_array);

    if (changed)
        return this;
    return NilObject::the();
}

Value ArrayObject::max(Env *env, Value count, Block *block) {
    if (m_vector.size() == 0)
        return NilObject::the();

    auto to_int = "to_int"_s;
    auto is_more = [&](Value item, Value min) -> bool {
        Value block_args[] = { item, min };
        Value compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, "<=>"_s, { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->inspect_str(),
                min->klass()->inspect_str());

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

    Vector<Value> maxes {};

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
    return new ArrayObject { maxes.slice(0, c) };
}

Value ArrayObject::min(Env *env, Value count, Block *block) {
    if (m_vector.size() == 0)
        return NilObject::the();

    auto to_int = "to_int"_s;
    auto is_less = [&](Value item, Value min) -> bool {
        Value block_args[] = { item, min };
        Value compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, "<=>"_s, { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->inspect_str(),
                min->klass()->inspect_str());

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

    Vector<Value> mins {};

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
    return new ArrayObject { mins.slice(0, c) };
}

Value ArrayObject::minmax(Env *env, Block *block) {
    if (m_vector.size() == 0)
        return new ArrayObject { NilObject::the(), NilObject::the() };

    auto to_int = "to_int"_s;
    auto compare = [&](Value item, Value min) -> nat_int_t {
        Value block_args[] = { item, min };
        Value compare = (block) ? NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, block_args, nullptr) : item.send(env, "<=>"_s, { min });

        if (compare->is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item->klass()->inspect_str(),
                min->klass()->inspect_str());

        if (!compare->is_integer() && compare->respond_to(env, to_int))
            compare = compare.send(env, to_int);

        return compare->as_integer()->to_nat_int_t();
    };

    Value max;
    Value min;

    for (auto item : *this) {
        if (max == nullptr || compare(item, max) > 0)
            max = item;
        if (min == nullptr || compare(item, min) < 0)
            min = item;
    }
    return new ArrayObject { min, max };
}

Value ArrayObject::multiply(Env *env, Value factor) {
    auto to_str = "to_str"_s;

    if (!factor->is_string() && factor->respond_to(env, to_str))
        factor = factor.send(env, to_str);

    if (factor->is_string()) {
        return join(env, factor);
    }

    auto to_int = "to_int"_s;

    if (!factor->is_integer() && factor->respond_to(env, to_int))
        factor = factor.send(env, to_int);

    if (factor->is_integer()) {
        auto times = factor->as_integer()->to_nat_int_t();

        if (times < 0)
            env->raise("ArgumentError", "negative argument");

        auto accumulator = new ArrayObject { times * size() };
        accumulator->m_klass = klass();

        for (nat_int_t i = 0; i < times; ++i)
            accumulator->push_splat(env, this);

        return accumulator;
    }

    env->raise("TypeError", "no implicit conversion of {} into Integer", factor->klass()->inspect_str());

    return this;
}

Value ArrayObject::compact(Env *env) {
    auto ary = new ArrayObject {};
    for (auto item : *this) {
        if (item->is_nil()) continue;
        ary->push(item);
    }
    return ary;
}

Value ArrayObject::compact_in_place(Env *env) {
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

    return NilObject::the();
}

Value ArrayObject::cycle(Env *env, Value count, Block *block) {
    // FIXME: delegating to Enumerable#cycle like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#cycle in MRI and it won't affect Array#cycle.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto none_method = Enumerable->find_method(env, "cycle"_s);
    return none_method->call(env, this, 1, &count, block);
}

Value ArrayObject::uniq(Env *env, Block *block) {
    ArrayObject *copy = new ArrayObject(*this);
    copy->uniq_in_place(env, block);
    return copy;
}

Value ArrayObject::uniq_in_place(Env *env, Block *block) {
    this->assert_not_frozen(env);

    auto hash = new HashObject {};
    for (auto item : *this) {
        Value key = item;
        if (block) {
            key = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, 1, &item, nullptr);
        }
        if (!hash->has_key(env, key)) {
            hash->put(env, key, item);
        }
    }

    ArrayObject *values = hash->values(env)->as_array();

    if (m_vector.size() == values->size())
        return NilObject::the();

    m_vector.clear();
    for (auto item : *values) {
        m_vector.push(item);
    }

    return this;
}

Value ArrayObject::clear(Env *env) {
    this->assert_not_frozen(env);
    m_vector.clear();
    return this;
}

Value ArrayObject::at(Env *env, Value n) {
    return ref(env, n, nullptr);
}

Value ArrayObject::assoc(Env *env, Value needle) {
    // TODO use common logic for this (see for example rassoc and index)
    for (auto &item : *this) {
        if (!item->is_array())
            continue;

        ArrayObject *sub_array = item->as_array();
        if (sub_array->is_empty())
            continue;

        if ((*sub_array)[0].send(env, "=="_s, { needle })->is_truthy())
            return sub_array;
    }

    return NilObject::the();
}

Value ArrayObject::bsearch(Env *env, Block *block) {
    if (!block) {
        return send(env, "enum_for"_s, { "bsearch"_s });
    }
    auto index = bsearch_index(env, block);
    if (index->is_nil())
        return index;

    return (*this)[index->as_integer()->to_nat_int_t()];
}

Value ArrayObject::bsearch_index(Env *env, Block *block) {
    if (!block) {
        return send(env, "enum_for"_s, { "bsearch_index"_s });
    }

    nat_int_t left = 0;
    nat_int_t right = m_vector.size() - 1;
    nat_int_t last_index { -1 };

    do {
        nat_int_t i = floor((left + right) / 2);
        auto outcome = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &(*this)[i], nullptr);
        if (!(
                outcome->is_numeric() || outcome->is_nil() || outcome->is_boolean())) {
            env->raise("TypeError", "wrong argument type {} (must be numeric, true, false or nil)", outcome->klass()->inspect_str());
        }

        if (outcome->is_numeric()) {
            auto result = (outcome->is_integer() ? outcome->as_integer()->to_nat_int_t()
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
        return NilObject::the();
    return new IntegerObject { last_index };
}

Value ArrayObject::rassoc(Env *env, Value needle) {
    for (auto &item : *this) {
        if (!item->is_array())
            continue;

        ArrayObject *sub_array = item->as_array();
        if (sub_array->size() < 2)
            continue;

        if (sub_array->at(1)->send(env, "=="_s, { needle })->is_truthy())
            return sub_array;
    }

    return NilObject::the();
}

Value ArrayObject::hash(Env *env) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return Value { NilObject::the() };

        HashBuilder hash {};
        auto hash_method = "hash"_s;
        auto to_int = "to_int"_s;

        for (size_t i = 0; i < size(); ++i) {
            auto item = (*this)[i];
            auto item_hash = item->send(env, hash_method);

            if (item_hash->is_nil())
                continue;

            // this allows us to return the same hash for recursive arrays:
            // a = []; a << a; a.hash == [a].hash # => true
            // a = []; a << a << a; a.hash == [a, a].hash # => true
            if (item->is_array() && size() == item->as_array()->size() && eql(env, item))
                continue;

            if (!item_hash->is_integer() && item_hash->respond_to(env, to_int))
                item_hash = item_hash->send(env, to_int);

            hash.append(item_hash->as_integer()->to_nat_int_t());
        }

        return Value::integer(hash.digest());
    });
}

Value ArrayObject::insert(Env *env, size_t argc, Value *args) {
    this->assert_not_frozen(env);

    if (argc == 1)
        return this;

    auto index_ptr = args[0];

    if (!index_ptr->is_integer()) {
        auto sym_to_int = "to_int"_s;

        if (index_ptr->respond_to(env, sym_to_int)) {
            index_ptr = index_ptr->send(env, sym_to_int);
        }
    }

    if (!index_ptr->is_integer())
        env->raise("TypeError", "no implicit conversion of {} into Integer", index_ptr->klass()->inspect_str());

    auto index = index_ptr->as_integer()->to_nat_int_t();
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
        m_vector.push(NilObject::the());
    }

    for (size_t i = 1; i < argc; i++) {
        m_vector.insert(i + size_t_index - 1, args[i]);
    }

    return this;
}

Value ArrayObject::intersection(Env *env, Value arg) {
    return intersection(env, 1, &arg);
}

bool ArrayObject::include_eql(Env *env, Value arg) {
    auto eql = "eql?"_s;
    for (auto &val : *this) {
        if (arg->object_id() == val->object_id() || arg->send(env, eql, { val })->is_truthy())
            return true;
    }
    return false;
}

Value ArrayObject::intersection(Env *env, size_t argc, Value *args) {
    auto *result = new ArrayObject { *this };
    result->uniq_in_place(env, nullptr);

    TM::Vector<ArrayObject *> arrays;

    for (size_t i = 0; i < argc; ++i) {
        auto &arg = args[i];
        ArrayObject *other_array = arg->to_ary(env);

        if (!other_array->is_empty())
            arrays.push(other_array);
    }

    if (result->is_empty()) return result;
    if (arrays.size() != argc) return new ArrayObject;

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

Value ArrayObject::union_of(Env *env, Value arg) {
    if (!arg->is_array()) {
        env->raise("TypeError", "no implicit conversion of {} into Array", arg->klass()->inspect_str());
        return nullptr;
    }

    auto *result = new ArrayObject();
    auto add_value = [&result, &env](Value &val) {
        if (!result->include(env, val)) {
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

Value ArrayObject::union_of(Env *env, size_t argc, Value *args) {
    auto *result = new ArrayObject(*this);

    // TODO: we probably want to make | call this instead of this way for optimization
    for (size_t i = 0; i < argc; i++) {
        auto &arg = args[i];
        result = result->union_of(env, arg)->as_array();
    }

    return result;
}

Value ArrayObject::unshift(Env *env, size_t argc, Value *args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < argc; i++) {
        m_vector.insert(i, args[i]);
    }
    return this;
}

Value ArrayObject::reverse(Env *env) {
    ArrayObject *copy = new ArrayObject(*this);
    copy->reverse_in_place(env);
    return copy;
}

Value ArrayObject::reverse_each(Env *env, Block *block) {
    if (!block) {
        auto enumerator = send(env, "enum_for"_s, { "reverse_each"_s });
        enumerator->ivar_set(env, "@size"_s, Value::integer(m_vector.size()));
        return enumerator;
    }

    for (size_t i = m_vector.size(); i > 0; --i) {
        auto obj = (*this)[i - 1];
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, nullptr);
    }

    return this;
}

Value ArrayObject::reverse_in_place(Env *env) {
    assert_not_frozen(env);

    for (size_t i = 0; i < size() / 2; i++) {
        std::swap(m_vector[i], m_vector[size() - 1 - i]);
    }
    return this;
}

Value ArrayObject::concat(Env *env, size_t argc, Value *args) {
    assert_not_frozen(env);

    ArrayObject *original = new ArrayObject(*this);

    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        concat(*arg->to_ary(env));
    }

    return this;
}

nat_int_t ArrayObject::_resolve_index(nat_int_t nat_index) const {
    nat_int_t index { nat_index };

    if (nat_index < 0)
        index += size();

    if (index < 0)
        return -1;

    if (static_cast<size_t>(index) >= size())
        return -1;

    return index;
}

Value ArrayObject::rindex(Env *env, Value object, Block *block) {
    if (!block && !object) return send(env, "enum_for"_s, { "rindex"_s });
    return find_index(env, object, block, true);
}

Value ArrayObject::fetch(Env *env, Value arg_index, Value default_value, Block *block) {
    auto to_int = "to_int"_s;
    Value index_obj = arg_index;

    if (!arg_index->is_integer()) {
        if (arg_index->respond_to(env, to_int)) {
            index_obj = arg_index->send(env, to_int);
            if (!index_obj->is_integer()) {
                auto arg_index_class = arg_index->klass()->inspect_str();
                env->raise("TypeError", "can't convert {} to Integer ({}#to_int gives {})", arg_index_class, arg_index_class, index_obj->klass()->inspect_str());
            }
        }
    }
    index_obj->assert_type(env, Type::Integer, "Integer");

    Value value;
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

Value ArrayObject::find_index(Env *env, Value object, Block *block, bool search_reverse) {
    if (object && block) env->warn("given block not used");
    assert(size() <= NAT_INT_MAX);

    auto length = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < length; i++) {
        nat_int_t index = search_reverse ? length - i - 1 : i;
        auto item = m_vector[index];
        if (object) {
            if (item.send(env, "=="_s, { object })->is_truthy())
                return Value::integer(index);
        } else {
            Value args[] = { item };
            auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
            length = static_cast<nat_int_t>(size());
            if (result->is_truthy())
                return Value::integer(index);
        }
    }
    return NilObject::the();
}

Value ArrayObject::none(Env *env, size_t argc, Value *args, Block *block) {
    // FIXME: delegating to Enumerable#none? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#none? in MRI and it won't affect Array#none?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto none_method = Enumerable->find_method(env, "none?"_s);
    return none_method->call(env, this, argc, args, block);
}

Value ArrayObject::one(Env *env, size_t argc, Value *args, Block *block) {
    // FIXME: delegating to Enumerable#one? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#one? in MRI and it won't affect Array#one?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto one_method = Enumerable->find_method(env, "one?"_s);
    return one_method->call(env, this, argc, args, block);
}

Value ArrayObject::product(Env *env, size_t argc, Value *args, Block *block) {
    Vector<ArrayObject *> arrays;
    arrays.push(this);
    for (size_t i = 0; i < argc; ++i)
        arrays.push(args[i]->to_ary(env));

    constexpr size_t max_size_t = std::numeric_limits<size_t>::max();
    size_t number_of_combinations = 1;
    for (auto &item : arrays) {
        if (item->size() == 0) {
            number_of_combinations = 0;
            break;
        }

        if (max_size_t / number_of_combinations < item->size())
            env->raise("RangeError", "too big to product");

        number_of_combinations *= item->size();
    }

    ArrayObject *products = new ArrayObject { number_of_combinations };
    for (size_t iteration = 0; iteration < number_of_combinations; ++iteration) {
        ArrayObject *product = new ArrayObject { arrays.size() };
        size_t remaining_iterations = iteration;
        size_t block_size = number_of_combinations;

        for (size_t i = 0; i < arrays.size(); i++) {
            auto item = arrays[i];

            block_size /= item->size();
            size_t times = remaining_iterations / block_size;
            product->push(item->at(times));

            remaining_iterations -= times * block_size;
        }

        products->push(product);
    }

    if (block) {
        if (products->size() > 0)
            products->each(env, block);
        return this;
    }

    return products;
}

Value ArrayObject::rotate(Env *env, Value val) {
    ArrayObject *copy = new ArrayObject(*this);
    copy->rotate_in_place(env, val);
    return copy;
}

Value ArrayObject::rotate_in_place(Env *env, Value val) {
    assert_not_frozen(env);
    nat_int_t count = 1;
    auto to_int = "to_int"_s;
    if (val) {
        if (!val->is_integer() && val->respond_to(env, to_int)) {
            val = val->send(env, to_int);
        } else if (!val->is_integer()) {
            env->raise("TypeError", "no implicit conversion of {} into Integer", val->klass()->inspect_str());
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
        Vector<Value> stack;

        count = count % size();

        for (nat_int_t i = 0; i < count; i++)
            stack.push(m_vector.pop_front());

        for (auto &rotated_val : stack)
            push(rotated_val);

    } else if (count < 0) {
        Vector<Value> stack;

        count = -count % size();
        for (nat_int_t i = 0; i < count; i++)
            stack.push(m_vector.pop());

        for (auto &rotated_val : stack)
            m_vector.push_front(rotated_val);
    }

    return this;
}

Value ArrayObject::slice_in_place(Env *env, Value index_obj, Value size) {
    this->assert_not_frozen(env);
    auto to_int = "to_int"_s;

    if (size) {
        if (!index_obj->is_integer() && index_obj->respond_to(env, to_int))
            index_obj = index_obj.send(env, to_int);

        index_obj->assert_type(env, ObjectType::Integer, "Integer");

        if (!size->is_integer() && size->respond_to(env, to_int))
            size = size.send(env, to_int);

        size->assert_type(env, ObjectType::Integer, "Integer");

        size->as_integer()->assert_fixnum(env);
    }

    if (index_obj->is_integer()) {
        index_obj->as_integer()->assert_fixnum(env);

        auto start = index_obj->as_integer()->to_nat_int_t();

        if (!size) {
            start = _resolve_index(start);
            if (start < 0 || start >= (nat_int_t)m_vector.size())
                return NilObject::the();

            Value item = (*this)[start];
            for (size_t i = start; i < this->size() - 1; i++) {
                (*this)[i] = (*this)[i + 1];
            }
            m_vector.pop();
            return item;
        }

        size->assert_type(env, ObjectType::Integer, "Integer");

        nat_int_t length = size->as_integer()->to_nat_int_t();

        if (length < 0)
            return NilObject::the();

        if (start > (nat_int_t)m_vector.size())
            return NilObject::the();

        if (start == (nat_int_t)m_vector.size())
            return new ArrayObject {};

        return _slice_in_place(start, _resolve_index(start) + length, true);
    }

    if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();
        Value begin_obj = range->begin();
        nat_int_t start;

        if (begin_obj->is_nil()) {
            start = 0;
        } else {
            if (!begin_obj->is_integer() && begin_obj->respond_to(env, to_int))
                begin_obj = begin_obj.send(env, to_int);

            begin_obj->assert_type(env, ObjectType::Integer, "Integer");
            begin_obj->as_integer()->assert_fixnum(env);

            start = begin_obj->as_integer()->to_nat_int_t();
        }

        Value end_obj = range->end();

        nat_int_t end;

        if (end_obj->is_nil()) {
            end = this->size();
        } else {
            if (!end_obj->is_integer() && end_obj->respond_to(env, to_int))
                end_obj = end_obj.send(env, to_int);

            end_obj->assert_type(env, Type::Integer, "Integer");
            end_obj->as_integer()->assert_fixnum(env);
            end = end_obj->as_integer()->to_nat_int_t();
        }

        return _slice_in_place(start, end, range->exclude_end());
    }

    if (!index_obj->is_integer() && index_obj->respond_to(env, to_int))
        index_obj = index_obj.send(env, to_int);

    index_obj->assert_type(env, ObjectType::Integer, "Integer");

    return slice_in_place(env, index_obj, size);
}

Value ArrayObject::_slice_in_place(nat_int_t start, nat_int_t end, bool exclude_end) {
    if (start > (nat_int_t)m_vector.size())
        return NilObject::the();

    if (start == (nat_int_t)m_vector.size())
        return new ArrayObject {};

    start = _resolve_index(start);

    if (start < 0)
        return NilObject::the();

    if (end < 0)
        end = _resolve_index(end);

    nat_int_t length = end - start + (exclude_end ? 0 : 1);

    if (length < 0)
        length = 0;

    if (length + start > (nat_int_t)this->size())
        length = this->size() - start;

    if (length < 0)
        return NilObject::the();

    ArrayObject *newArr = new ArrayObject();
    if (length == 0) {
        return newArr;
    }

    if (start + length > (nat_int_t)this->size()) {
        length = this->size() - start;
    }

    newArr->m_vector.set_capacity(length);
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

Value ArrayObject::to_h(Env *env, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto to_h_method = Enumerable->find_method(env, "to_h"_s);
    return to_h_method->call(env, this, 0, nullptr, block);
}

Value ArrayObject::try_convert(Env *env, Value val) {
    auto to_ary = "to_ary"_s;

    if (val->is_array()) {
        return val;
    }

    if (!val->respond_to(env, to_ary)) {
        return NilObject::the();
    }

    auto conversion = val->send(env, to_ary);

    if (conversion->is_array() || conversion->is_nil()) {
        return conversion;
    }

    auto original_item_class_name = val->klass()->inspect_str();
    auto new_item_class_name = conversion->klass()->inspect_str();
    env->raise(
        "TypeError",
        "can't convert {} to Array ({}#to_ary gives {})",
        original_item_class_name,
        original_item_class_name,
        new_item_class_name);
}

Value ArrayObject::values_at(Env *env, size_t argc, Value *args) {
    TM::Vector<nat_int_t> indices;

    auto to_int = "to_int"_s;
    auto convert_to_int = [&](Value arg) -> nat_int_t {
        if (!arg->is_integer() && arg->respond_to(env, to_int)) {
            arg = arg.send(env, to_int);
        }
        arg->assert_type(env, Type::Integer, "Integer");
        return arg->as_integer()->to_nat_int_t();
    };

    for (size_t i = 0; i < argc; ++i) {
        auto arg = args[i];
        if (arg->is_range()) {
            auto begin_value = arg->as_range()->begin();
            auto end_value = arg->as_range()->end();
            nat_int_t begin, end;

            if (begin_value->is_nil()) {
                begin = 0;
            } else {
                begin = convert_to_int(begin_value);
                if (begin < 0)
                    break;
            }

            if (end_value->is_nil()) {
                end = size();
            } else {
                end = convert_to_int(end_value);
                if (!arg->as_range()->exclude_end())
                    end += 1;
                if (end < 0)
                    end += size();
                if (end < begin)
                    break;
            }

            for (nat_int_t j = begin; j < end; ++j)
                indices.push(j);
        } else {
            indices.push(convert_to_int(arg));
        }
    }

    auto accumulator = new ArrayObject { indices.size() };
    for (auto index : indices) {
        auto resolved_index = _resolve_index(index);
        if (resolved_index < 0 || static_cast<size_t>(resolved_index) >= m_vector.size()) {
            accumulator->push(NilObject::the());
            continue;
        }
        accumulator->push((*this)[resolved_index]);
    }

    return accumulator;
}

Value ArrayObject::zip(Env *env, size_t argc, Value *args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s)->as_module();
    auto zip_method = Enumerable->find_method(env, "zip"_s);
    return zip_method->call(env, this, argc, args, block);
}
}
