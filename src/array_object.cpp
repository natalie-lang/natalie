#include "natalie.hpp"
#include "natalie/bsearch.hpp"
#include "natalie/integer_methods.hpp"
#include <algorithm>
#include <math.h>
#include <natalie/array_object.hpp>
#include <natalie/array_packer/packer.hpp>
#include <natalie/string_object.hpp>
#include <natalie/symbol_object.hpp>
#include <tm/hashmap.hpp>
#include <tm/recursion_guard.hpp>

namespace Natalie {

ArrayObject::ArrayObject(std::initializer_list<Value> list)
    : ArrayObject {} {
    m_vector.set_capacity(list.size());
    for (auto v : list) {
        NAT_GC_GUARD_VALUE(v);
        m_vector.push(v);
    }
}

Value ArrayObject::initialize(Env *env, Optional<Value> size_arg, Optional<Value> value_arg, Block *block) {
    this->assert_not_frozen(env);

    if (!size_arg && !value_arg && block)
        env->verbose_warn("given block not used");

    if (!size_arg)
        return initialize_copy(env, ArrayObject::create());

    auto size = size_arg.value();

    if (!value_arg) {
        auto to_ary = "to_ary"_s;
        if (!size.is_array() && size.respond_to(env, to_ary))
            size = size.send(env, to_ary);

        if (size.is_array())
            return initialize_copy(env, size);
    }

    auto size_integer = size.to_int(env);
    if (size_integer.is_bignum())
        env->raise("ArgumentError", "array size too big");

    auto s = size_integer.to_nat_int_t();

    if (s < 0)
        env->raise("ArgumentError", "negative argument");

    if (block) {
        if (value_arg)
            env->warn("block supersedes default value argument");

        ArrayObject new_array;
        *this = std::move(new_array);

        for (nat_int_t i = 0; i < s; i++) {
            Value args[] = { Value::integer(i) };
            push(block->run(env, Args(1, args), nullptr));
        }

    } else {
        auto value = value_arg.value_or(Value::nil());
        for (nat_int_t i = 0; i < s; i++) {
            push(value);
        }
    }
    return this;
}

Value ArrayObject::initialize_copy(Env *env, Value other) {
    assert_not_frozen(env);

    ArrayObject *other_array = other.to_ary(env);
    if (this == other_array)
        return this;

    clear(env);
    m_vector.concat(other_array->m_vector);
    return this;
}

ArrayObject *ArrayObject::to_a() {
    if (klass() == GlobalEnv::the()->Array()) {
        return this;
    } else {
        return ArrayObject::create(m_vector);
    }
}

void ArrayObject::push(Value val) {
    NAT_GC_GUARD_VALUE(val);
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_vector.push(val);
}

Value ArrayObject::first() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_vector.is_empty())
        return Value::nil();
    return m_vector[0];
}

Value ArrayObject::last() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_vector.is_empty())
        return Value::nil();
    return m_vector[m_vector.size() - 1];
}

Value ArrayObject::pop() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_vector.is_empty())
        return Value::nil();
    return m_vector.pop();
}

Value ArrayObject::shift() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_vector.is_empty())
        return Value::nil();
    return m_vector.pop_front();
}

void ArrayObject::set(size_t index, Value value) {
    NAT_GC_GUARD_VALUE(value);
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (index == m_vector.size()) {
        m_vector.push(value);
        return;
    }

    if (index > m_vector.size()) {
        m_vector.set_size(index, Value::nil());
        m_vector.push(value);
        return;
    }

    m_vector[index] = value;
}

Value ArrayObject::inspect(Env *env) {
    RecursionGuard guard { this };

    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return StringObject::create("[...]");

        if (is_empty())
            return StringObject::create("[]", Encoding::US_ASCII);

        StringObject *out = StringObject::create("[");
        for (size_t i = 0; i < size(); i++) {
            Value obj = (*this)[i];

            auto inspected_repr = obj.send(env, "inspect"_s);

            if (!inspected_repr.is_string()) {
                SymbolObject *to_s = "to_s"_s;
                if (inspected_repr.respond_to(env, to_s))
                    inspected_repr = obj.send(env, to_s);
            }

            if (inspected_repr.is_string())
                out->append(inspected_repr.as_string());
            else
                out->append_sprintf("#<%s:%#x>", inspected_repr.klass()->inspect_module().c_str(), inspected_repr.object_id());

            if (i < size() - 1) {
                out->append(", ");
            }
        }
        out->append_char(']');
        return out;
    });
}

String ArrayObject::dbg_inspect(int indent) const {
    auto str = String::format("<ArrayObject {h} size={} data=[", this, size());
    for (size_t index = 0; index < size(); index++) {
        str.append_char('\n');
        str.append_char(' ', indent + 2);
        auto item = (*this)[index];
        str.append(item.dbg_inspect(indent + 2));
        if (index < size() - 1)
            str.append_char(',');
    }
    str.append("]>");
    return str;
}

Value ArrayObject::ltlt(Env *env, Value arg) {
    this->assert_not_frozen(env);
    push(arg);
    return this;
}

Value ArrayObject::add(Env *env, Value other) {
    ArrayObject *other_array = other.to_ary(env);

    ArrayObject *new_array = ArrayObject::create(m_vector);
    new_array->concat(*other_array);
    return new_array;
}

Value ArrayObject::sub(Env *env, Value other) {
    ArrayObject *other_array = other.to_ary(env);

    ArrayObject *new_array = ArrayObject::create();
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other_array) {
            if ((item.send(env, "eql?"_s, { compare_item }).is_truthy() && item.send(env, "hash"_s).send(env, "eql?"_s, { compare_item.send(env, "hash"_s) }).is_truthy())
                || item.send(env, "=="_s, { compare_item }).is_truthy()) {
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

Value ArrayObject::sum(Env *env, Args &&args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "sum"_s);
    return method_info.method()->call(env, this, std::move(args), block);
}

Value ArrayObject::ref(Env *env, Value index_obj, Optional<Value> size) {
    if (!size) {
        if (!index_obj.is_integer() && index_obj.respond_to(env, "to_int"_s))
            index_obj = index_obj.send(env, "to_int"_s);

        if (index_obj.is_integer()) {
            auto index = _resolve_index(IntegerMethods::convert_to_nat_int_t(env, index_obj));
            if (index < 0 || index >= (nat_int_t)m_vector.size())
                return Value::nil();
            return m_vector[index];
        }
    }

    ArrayObject *copy = ArrayObject::create(*this);
    return copy->slice_in_place(env, index_obj, size);
}

Value ArrayObject::refeq(Env *env, Value index_obj, Value size, Optional<Value> val_arg) {
    this->assert_not_frozen(env);
    nat_int_t start, width;
    if (index_obj.is_range()) {
        RangeObject *range = index_obj.as_range();
        Value begin_obj = range->begin();
        Value end_obj = range->end();

        // Ignore "size"
        val_arg = size;

        if (begin_obj.is_nil()) {
            start = 0;
        } else {
            start = IntegerMethods::convert_to_nat_int_t(env, begin_obj);
            if (start < 0) {
                if ((size_t)(-start) > this->size())
                    env->raise("RangeError", "{} out of range", range->inspected(env));
                start = this->size() + start;
            }
        }

        nat_int_t end;
        if (end_obj.is_nil()) {
            end = this->size();
        } else {
            end = IntegerMethods::convert_to_nat_int_t(env, end_obj);
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
        start = IntegerMethods::convert_to_nat_int_t(env, index_obj);
        if (start < 0) {
            if ((size_t)(-start) > this->size())
                env->raise("IndexError", "index {} too small for array; minimum: -{}", start, this->size());
            start = this->size() + start;
        }

        Value val;
        if (val_arg) {
            val = val_arg.value();
        } else {
            val = size;
            if (start < (nat_int_t)this->size()) {
                (*this)[start] = val;
            } else {
                expand_with_nil(env, start);
                push(val);
            }
            return val;
        }

        width = IntegerMethods::convert_to_nat_int_t(env, size);
        if (width < 0)
            env->raise("IndexError", "negative length ({})", width);
    }

    // PERF: inefficient for large arrays where changes are being made to only the right side
    Vector<Value> new_ary {};

    // stuff before the new entry/entries
    new_ary.concat_slice(m_vector, 0, std::min(this->size(), static_cast<size_t>(start)));

    // extra nils if needed
    if (start > static_cast<nat_int_t>(this->size()))
        new_ary.set_size(start, Value::nil());

    // the new entry/entries
    Value val;
    if (val_arg) {
        val = val_arg.value();
        if (val.is_array() || val.respond_to(env, "to_ary"_s)) {
            new_ary.concat(val.to_ary(env)->m_vector);
        } else {
            new_ary.push(val);
        }
    } else {
        NAT_UNREACHABLE();
    }

    // stuff after the new entry/entries
    if (start + width < static_cast<nat_int_t>(this->size()))
        new_ary.concat_slice(m_vector, start + width, this->size());

    m_vector = std::move(new_ary);

    return val;
}

Value ArrayObject::any(Env *env, Args &&args, Block *block) {
    // FIXME: delegating to Enumerable#any? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#any? in MRI and it won't affect Array#any?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "any?"_s);
    return method_info.method()->call(env, this, std::move(args), block);
}

bool ArrayObject::eq(Env *env, Value other) {
    auto lambda = [&](bool is_recursive) {
        if (other == this)
            return true;

        SymbolObject *equality = "=="_s;
        if (!other.is_array()
            && other.send(env, "respond_to?"_s, { "to_ary"_s }).is_truthy())
            return other.send(env, equality, { this }).is_truthy();

        if (!other.is_array())
            return false;

        auto other_array = other.as_array();
        if (size() != other_array->size())
            return false;

        if (is_recursive)
            // since == is an & of all the == of each value, this will just leave the expression uneffected
            return true;

        SymbolObject *object_id = "object_id"_s;
        for (size_t i = 0; i < size(); ++i) {
            Value this_item = (*this)[i];
            Value item = (*other_array)[i];

            if (this_item.respond_to(env, object_id) && item.respond_to(env, object_id)) {
                Value same_object_id = this_item
                                           .send(env, object_id)
                                           .send(env, equality, { item.send(env, object_id) });

                // This allows us to check NAN equality and other potentially similar constants
                if (same_object_id.is_true())
                    continue;
            }

            Value result = this_item.send(env, equality, { item }, nullptr);
            if (result.is_falsey())
                return false;
        }

        return true;
    };

    if (!other.has_heap_object())
        return lambda(false);

    TM::PairedRecursionGuard guard { this, other.object() };
    return guard.run(lambda);
}

bool ArrayObject::eql(Env *env, Value other) {
    auto lambda = [&](bool is_recursive) {
        if (other == this)
            return true;
        if (!other.is_array())
            return false;

        if (!other.is_array()) {
            return false;
        }

        auto other_array = other.as_array();
        if (size() != other_array->size())
            return false;

        if (is_recursive)
            // since eql is an & of all the eql of each value, this will just leave the expression uneffected
            return true;

        for (size_t i = 0; i < size(); ++i) {
            Value result = (*this)[i].send(env, "eql?"_s, { (*other_array)[i] }, nullptr);
            if (result.is_false())
                return false;
        }

        return true;
    };

    if (!other.has_heap_object())
        return lambda(false);

    TM::PairedRecursionGuard guard { this, other.object() };
    return guard.run(lambda);
}

Value ArrayObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }

    for (size_t i = 0; i < size(); ++i) {
        Value args[] = { (*this)[i] };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value ArrayObject::each_index(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "each_index"_s }, size_block);
    }

    nat_int_t size_nat_int_t = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < size_nat_int_t; i++) {
        Value args[] = { Value::integer(i) };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value ArrayObject::map(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "map"_s }, size_block);
    }

    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->map_in_place(env, block);
    return copy;
}

Value ArrayObject::map_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "map!"_s }, size_block);
    }

    assert_not_frozen(env);

    for (size_t i = 0; i < m_vector.size(); ++i) {
        Value args[] = { (*this)[i] };
        m_vector.at(i) = block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value ArrayObject::fill(Env *env, Optional<Value> obj_arg, Optional<Value> start_arg, Optional<Value> length_arg, Block *block) {
    assert_not_frozen(env);

    if (block && length_arg)
        env->raise("ArgumentError", "wrong number of arguments (given 3, expected 0..2)");

    if (!obj_arg && !block)
        env->raise("ArgumentError", "wrong number of arguments (given 0, expected 1..3)");

    if (!length_arg && block) {
        length_arg = start_arg;
        start_arg = obj_arg;
    }

    nat_int_t start = 0;
    nat_int_t max = size();

    if (start_arg && !start_arg.value().is_nil()) {
        auto start_obj = start_arg.value();
        if (!length_arg && start_arg.value().is_range()) {

            Value begin = start_obj.as_range()->begin();
            if (!begin.is_nil()) {
                start = IntegerMethods::convert_to_nat_int_t(env, begin);

                if (start < 0)
                    start += size();
                if (start < 0)
                    env->raise("RangeError", "{} out of range", start_obj.inspected(env));
            }

            auto end = start_obj.as_range()->end();

            if (!end.is_nil()) {
                max = IntegerMethods::convert_to_nat_int_t(env, end);

                if (max < 0)
                    max += size();
                if (max != 0 && !start_obj.as_range()->exclude_end())
                    ++max;
            }
        } else {
            start = IntegerMethods::convert_to_nat_int_t(env, start_obj);

            if (start < 0)
                start += size();
            if (start < 0)
                start = 0;

            if (length_arg && !length_arg.value().is_nil()) {
                auto length = IntegerMethods::convert_to_nat_int_t(env, length_arg.value());
                if (length >= 2LL << 60)
                    env->raise("ArgumentError", "argument too big");

                if (length <= 0)
                    return this;

                if (__builtin_add_overflow(start, length, &max))
                    env->raise("ArgumentError", "argument too big");
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
            m_vector[i] = block->run(env, Args(1, args), nullptr);
        } else
            m_vector[i] = obj_arg.value();
    }
    return this;
}

Value ArrayObject::first(Env *env, Optional<Value> n) {
    if (!n) {
        if (is_empty())
            return Value::nil();

        return (*this)[0];
    }

    nat_int_t n_value = IntegerMethods::convert_to_nat_int_t(env, n.value());

    if (n_value < 0)
        env->raise("ArgumentError", "negative array size");

    size_t end = std::min(size(), (size_t)n_value);
    if (end == 0)
        return ArrayObject::create();
    auto array = m_vector.slice(0, end);
    return ArrayObject::create(std::move(array));
}

Value ArrayObject::flatten(Env *env, Optional<Value> depth) {
    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->flatten_in_place(env, depth);
    return copy;
}

Value ArrayObject::flatten_in_place(Env *env, Optional<Value> depth) {
    this->assert_not_frozen(env);

    bool changed { false };

    if (depth) {
        auto depth_value = IntegerMethods::convert_to_nat_int_t(env, depth.value());
        changed = _flatten_in_place(env, depth_value);
    } else {
        changed = _flatten_in_place(env, -1);
    }

    if (changed)
        return this;

    return Value::nil();
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

        if (!item.is_array()) {
            Value new_item = try_convert(env, item);

            if (!new_item.is_nil()) {
                item = new_item;
            }
        }

        if (item.is_array()) {
            changed = true;
            m_vector.remove(i - 1);

            auto array_item = item.as_array();

            if (visited.get(array_item) != nullptr) {
                env->raise("ArgumentError", "tried to flatten recursive array");
                return false;
            }

            // add current array item so if it is referenced by any of its children a loop is detected
            visited.set(array_item);

            // use a copy so we avoid altering the content of nested arrays
            ArrayObject *copy = ArrayObject::create(*array_item);

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

    auto nat_resolved_index = _resolve_index(IntegerMethods::convert_to_nat_int_t(env, n));
    if (nat_resolved_index < 0)
        return Value::nil();

    auto resolved_index = static_cast<size_t>(nat_resolved_index);
    auto value = (*this)[resolved_index];
    m_vector.remove(resolved_index);
    return value;
}

Value ArrayObject::delete_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "delete_if"_s }, size_block);
    }

    this->assert_not_frozen(env);

    Vector<size_t> marked_indexes;

    for (size_t i = 0; i < size(); ++i) {
        Value args[] = { (*this)[i] };
        Value result = block->run(env, Args(1, args), nullptr);
        if (result.is_truthy()) {
            marked_indexes.push(i);
        }
    }

    while (!marked_indexes.is_empty()) {
        m_vector.remove(marked_indexes.pop());
    }

    return this;
}

Value ArrayObject::delete_item(Env *env, Value target, Block *block) {
    Value deleted_item = Value::nil();

    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];
        if (Object::neq(env, item, target))
            continue;

        if (deleted_item.is_nil()) {
            // frozen assertion only happens if any item needs to in fact be deleted
            this->assert_not_frozen(env);
            deleted_item = item;
        }

        m_vector.remove(i - 1);
    }

    if (deleted_item.is_nil() && block) {
        return block->run(env, {}, nullptr);
    }

    return deleted_item;
}

Value ArrayObject::difference(Env *env, Args &&args) {
    Value last = ArrayObject::create(m_vector);

    for (size_t i = 0; i < args.size(); i++) {
        last = last.as_array()->sub(env, args[i]);
    }

    return last;
}

Value ArrayObject::dig(Env *env, Args &&args) {
    args.ensure_argc_at_least(env, 1);
    auto dig = "dig"_s;
    auto idx = args.shift(env, true);
    Value val = ref(env, idx);
    if (args.size() == 0)
        return val;

    if (val == Value::nil())
        return val;

    if (!val.respond_to(env, dig))
        env->raise("TypeError", "{} does not have #dig method", val.klass()->inspect_module());

    return val.send(env, dig, std::move(args));
}

Value ArrayObject::drop(Env *env, Value n) {
    auto n_value = IntegerMethods::convert_to_nat_int_t(env, n);

    if (n_value < 0)
        env->raise("ArgumentError", "attempt to drop negative size");

    auto array = m_vector.slice(n_value, 0);
    return ArrayObject::create(std::move(array));
}

Value ArrayObject::drop_while(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "drop_while"_s });

    size_t i = 0;
    while (i < m_vector.size()) {
        Value args[] = { m_vector.at(i) };
        Value result = block->run(env, Args(1, args), nullptr);
        if (result.is_nil() || result.is_false()) {
            break;
        }

        ++i;
    }

    auto array = m_vector.slice(i, 0);
    return ArrayObject::create(std::move(array));
}

Value ArrayObject::last(Env *env, Optional<Value> n) {
    if (!n) {
        if (is_empty())
            return Value::nil();

        return (*this)[size() - 1];
    }

    auto n_value = IntegerMethods::convert_to_nat_int_t(env, n.value());

    if (n_value < 0)
        env->raise("ArgumentError", "negative array size");

    assert(size() <= NAT_INT_MAX);
    nat_int_t signed_size = static_cast<nat_int_t>(size());
    size_t start = std::max(static_cast<nat_int_t>(0), signed_size - n_value);

    auto array = m_vector.slice(start, 0);
    return ArrayObject::create(std::move(array));
}

bool ArrayObject::include(Env *env, Value item) {
    if (size() == 0) {
        return false;
    } else {
        for (auto &compare_item : *this) {
            if (compare_item.send(env, "=="_s, { item }).is_truthy()) {
                return true;
            }
        }
        return false;
    }
}

Value ArrayObject::index(Env *env, Optional<Value> object, Block *block) {
    if (!block && !object) return send(env, "enum_for"_s, { "index"_s });
    return find_index(env, object, block);
}

Value ArrayObject::shift(Env *env, Optional<Value> count) {
    assert_not_frozen(env);

    if (!count && is_empty())
        return Value::nil();

    size_t shift_count = 1;
    Value result;
    if (count) {
        auto count_signed = IntegerMethods::convert_to_nat_int_t(env, count.value());

        if (count_signed < 0)
            env->raise("ArgumentError", "negative array size");

        shift_count = count_signed;

        if (shift_count == 0) {
            return ArrayObject::create();
        }
        result = ArrayObject::create(m_vector.slice(0, shift_count));
    } else {
        result = m_vector[0];
    }

    auto shifted = m_vector.slice(shift_count);
    m_vector = std::move(shifted);
    return result;
}

Value ArrayObject::sort(Env *env, Block *block) {
    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->sort_in_place(env, block);
    return copy;
}

Value ArrayObject::keep_if(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "keep_if"_s }, size_block);
    }

    select_in_place(env, block);
    return this;
}

Value ArrayObject::_subjoin(Env *env, Value item, Value joiner) {
    if (item.is_string()) {
        return item.as_string();
    } else if (item.is_array()) {
        return item.as_array()->join(env, joiner).as_string();
    } else {
        auto to_str = "to_str"_s;
        auto to_ary = "to_ary"_s;
        auto to_s = "to_s"_s;
        if (item.is_integer())
            return item.send(env, to_s);
        if (item.respond_to(env, to_str)) {
            // Need to support nil, don't use Object::to_str
            auto rval = item.send(env, to_str);
            if (!rval.is_nil()) return rval.as_string();
        }
        if (item.respond_to(env, to_ary)) {
            // Need to support nil, don't use Object::to_ary
            auto rval = item.send(env, to_ary);
            if (!rval.is_nil()) return rval.as_array()->join(env, joiner).as_string();
        }
        if (item.respond_to(env, to_s))
            item = item.send(env, to_s);
        if (item.is_string())
            return item.as_string();
    }
    env->raise("NoMethodError", "needed to_str, to_ary, or to_s");
}

Value ArrayObject::join(Env *env, Optional<Value> joiner_arg) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            env->raise("ArgumentError", "recursive array join");
        if (size() == 0) {
            return (Value)StringObject::create("", Encoding::US_ASCII);
        } else {
            Value joiner;
            if (joiner_arg && !joiner_arg.value().is_nil())
                joiner = joiner_arg.value();
            else
                joiner = env->global_get("$,"_s);
            if (joiner.is_nil())
                joiner = StringObject::create("");

            if (!joiner.is_string())
                joiner = joiner.to_str(env);

            StringObject *out = StringObject::create();
            for (size_t i = 0; i < size(); i++) {
                Value item = (*this)[i];
                out->append(_subjoin(env, item, joiner));
                if (i < (size() - 1))
                    out->append(joiner.as_string());
            }
            return (Value)out;
        }
    });
}

Value ArrayObject::cmp(Env *env, Value other) {
    Value other_converted = try_convert(env, other);

    if (other_converted.is_nil()) {
        return other_converted;
    }

    ArrayObject *other_array = other_converted.as_array();
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

            if (!cmp_obj.is_integer()) {
                return cmp_obj;
            }

            auto cmp = cmp_obj.integer();
            if (cmp < 0) return Value::integer(-1);
            if (cmp > 0) return Value::integer(1);
        }
        if (other_array->size() > size()) {
            return Value::integer(-1);
        }
        return Value::integer(0);
    });
}

Value ArrayObject::pack(Env *env, Value directives, Optional<Value> buffer_arg) {
    if (!directives.is_string())
        directives = directives.to_str(env);

    auto directives_string = directives.as_string()->string();
    if (directives_string.is_empty())
        return StringObject::create("", Encoding::US_ASCII);

    if (buffer_arg) {
        auto buffer = buffer_arg.value();
        if (!buffer.is_string())
            env->raise("TypeError", "buffer must be String, not {}", buffer.klass()->inspect_module());
        return ArrayPacker::Packer { this, directives_string }.pack(env, buffer.as_string());
    } else {
        StringObject *start_buffer = StringObject::create("", Encoding::ASCII_8BIT);
        return ArrayPacker::Packer { this, directives_string }.pack(env, start_buffer);
    }
}

Value ArrayObject::push(Env *env, Args &&args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < args.size(); i++) {
        push(args[i]);
    }
    return this;
}

void ArrayObject::push_splat(Env *env, Value val) {
    if (!val.is_array() && val.respond_to(env, "to_a"_s)) {
        val = val.send(env, "to_a"_s);
    }
    if (val.is_array()) {
        m_vector.concat(val.as_array()->m_vector);
    } else {
        push(val);
    }
}

Value ArrayObject::pop(Env *env, Optional<Value> count) {
    this->assert_not_frozen(env);

    if (count) {
        auto c = IntegerMethods::convert_to_nat_int_t(env, count.value());

        if (c < 0)
            env->raise("ArgumentError", "negative array size");

        if (c > (nat_int_t)size())
            c = (nat_int_t)size();

        auto pops = ArrayObject::create((size_t)c);
        for (nat_int_t i = 0; i < c; ++i)
            pops->m_vector.push_front(m_vector.pop());

        return pops;
    }
    if (size() == 0) return Value::nil();
    Value val = m_vector.pop();
    return val;
}

void ArrayObject::expand_with_nil(Env *env, size_t total) {
    for (size_t i = size(); i < total; i++) {
        push(Value::nil());
    }
}

bool array_sort_compare(Env *env, Value a, Value b, Block *block) {
    if (block) {
        Value args[2] = { a, b };
        Value compare = block->run(env, Args(2, args), nullptr);

        if (compare.respond_to(env, "<"_s)) {
            Value zero = Value::integer(0);
            return compare.send(env, "<"_s, { zero }).is_truthy();
        } else {
            env->raise("ArgumentError", "comparison of {} with 0 failed", compare.klass()->inspect_module());
        }
    } else {
        Value compare = a.send(env, "<=>"_s, { b });
        if (compare.is_integer()) {
            return compare.integer() < 0;
        }
        // TODO: Ruby sometimes prints b as the value (for example for integers) and sometimes as class
        env->raise("ArgumentError", "comparison of {} with {} failed", a.klass()->inspect_module(), b.klass()->inspect_module());
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
    Value args[1];
    args[0] = a;
    Value a_res = block->run(env, Args(1, args), nullptr);

    args[0] = b;
    Value b_res = block->run(env, Args(1, args), nullptr);

    Value compare = a_res.send(env, "<=>"_s, { b_res });
    if (compare.is_integer()) {
        return compare.integer() < 0;
    }
    env->raise("ArgumentError", "comparison of {} with {} failed", a_res.klass()->inspect_module(), b_res.klass()->inspect_module());
}

Value ArrayObject::sort_by_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "sort_by!"_s }, size_block);
    }

    this->assert_not_frozen(env);

    m_vector.sort([env, block](Value a, Value b) {
        return array_sort_by_compare(env, a, b, block);
    });

    return this;
}

Value ArrayObject::select(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "select"_s }, size_block);
    }

    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->select_in_place(env, block);
    return copy;
}

Value ArrayObject::select_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "select!"_s }, size_block);
    }

    assert_not_frozen(env);

    bool changed = select_in_place([env, block](Value &item) -> bool {
        Value args[] = { item };
        Value result = block->run(env, Args(1, args), nullptr);
        return result.is_truthy();
    });

    if (changed)
        return this;
    return Value::nil();
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
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "reject"_s }, size_block);
    }

    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->reject_in_place(env, block);
    return copy;
}

Value ArrayObject::reject_in_place(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "reject!"_s }, size_block);
    }

    assert_not_frozen(env);

    bool changed { false };

    ArrayObject new_array;

    for (auto it = begin(); it != end(); it++) {
        auto item = *it;

        try {
            Value args[] = { item };
            Value result = block->run(env, Args(1, args), nullptr);
            if (result.is_falsey())
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
    return Value::nil();
}

Value ArrayObject::max(Env *env, Optional<Value> count, Block *block) {
    if (m_vector.size() == 0)
        return Value::nil();

    auto is_more = [&](Value item, Value min) -> bool {
        Value block_args[] = { item, min };
        Value compare = (block) ? block->run(env, Args(2, block_args), nullptr) : item.send(env, "<=>"_s, { min });

        if (compare.is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item.klass()->inspect_module(),
                min.klass()->inspect_module());

        auto nat_int = IntegerMethods::convert_to_nat_int_t(env, compare);
        return nat_int > 0;
    };

    auto has_implicit_count = !count || count.value().is_nil();
    size_t c;

    if (has_implicit_count) {
        c = 1;
    } else {
        auto c_nat_int = IntegerMethods::convert_to_nat_int_t(env, count.value());
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
    return ArrayObject::create(maxes.slice(0, c));
}

Value ArrayObject::min(Env *env, Optional<Value> count, Block *block) {
    if (m_vector.size() == 0)
        return Value::nil();

    auto is_less = [&](Value item, Value min) -> bool {
        Value block_args[] = { item, min };
        Value compare = (block) ? block->run(env, Args(2, block_args), nullptr) : item.send(env, "<=>"_s, { min });

        if (compare.is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item.klass()->inspect_module(),
                min.klass()->inspect_module());

        auto nat_int = IntegerMethods::convert_to_nat_int_t(env, compare);
        return nat_int < 0;
    };

    auto has_implicit_count = !count || count.value().is_nil();
    size_t c;

    if (has_implicit_count) {
        c = 1;
    } else {
        auto c_nat_int = IntegerMethods::convert_to_nat_int_t(env, count.value());
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
    return ArrayObject::create(mins.slice(0, c));
}

Value ArrayObject::minmax(Env *env, Block *block) {
    if (m_vector.size() == 0)
        return ArrayObject::create({ Value::nil(), Value::nil() });

    auto compare = [&](Value item, Value min) -> nat_int_t {
        Value block_args[] = { item, min };
        Value compare = (block) ? block->run(env, Args(2, block_args), nullptr) : item.send(env, "<=>"_s, { min });

        if (compare.is_nil())
            env->raise(
                "ArgumentError",
                "comparison of {} with {} failed",
                item.klass()->inspect_module(),
                min.klass()->inspect_module());

        auto nat_int = IntegerMethods::convert_to_nat_int_t(env, compare);
        return nat_int;
    };

    Optional<Value> max;
    Optional<Value> min;

    for (auto item : *this) {
        if (!max || compare(item, max.value()) > 0)
            max = item;
        if (!min || compare(item, min.value()) < 0)
            min = item;
    }
    return ArrayObject::create({ min.value(), max.value() });
}

Value ArrayObject::multiply(Env *env, Value factor) {
    if (!factor.is_string() && factor.respond_to(env, "to_str"_s))
        factor = factor.to_str(env);

    if (factor.is_string()) {
        return join(env, factor);
    }

    auto times = IntegerMethods::convert_to_nat_int_t(env, factor);
    if (times < 0)
        env->raise("ArgumentError", "negative argument");

    auto accumulator = ArrayObject::create(times * size());

    for (nat_int_t i = 0; i < times; ++i)
        accumulator->push_splat(env, this);

    return accumulator;
}

Value ArrayObject::compact(Env *env) {
    auto ary = ArrayObject::create();
    for (auto item : *this) {
        if (item.is_nil()) continue;
        ary->push(item);
    }
    return ary;
}

Value ArrayObject::compact_in_place(Env *env) {
    this->assert_not_frozen(env);

    bool changed { false };
    for (size_t i = size(); i > 0; --i) {
        auto item = (*this)[i - 1];
        if (item.is_nil()) {
            changed = true;
            m_vector.remove(i - 1);
        }
    }

    if (changed)
        return this;

    return Value::nil();
}

Value ArrayObject::cycle(Env *env, Optional<Value> count, Block *block) {
    // FIXME: delegating to Enumerable#cycle like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#cycle in MRI and it won't affect Array#cycle.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "cycle"_s);
    auto args = count ? Vector<Value> { count.value() } : Vector<Value> {};
    return method_info.method()->call(env, this, { std::move(args) }, block);
}

Value ArrayObject::uniq(Env *env, Block *block) {
    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->uniq_in_place(env, block);
    return copy;
}

Value ArrayObject::uniq_in_place(Env *env, Block *block) {
    this->assert_not_frozen(env);

    auto hash = HashObject::create();
    for (auto item : *this) {
        Value key = item;
        if (block) {
            Value args[] = { item };
            key = block->run(env, Args(1, args), nullptr);
        }
        if (!hash->has_key(env, key)) {
            hash->put(env, key, item);
        }
    }

    ArrayObject *values = hash->values(env).as_array();

    if (m_vector.size() == values->size())
        return Value::nil();

    *this = std::move(*values);

    return this;
}

Value ArrayObject::clear(Env *env) {
    this->assert_not_frozen(env);
    m_vector.clear();
    return this;
}

Value ArrayObject::at(Env *env, Value n) {
    return ref(env, n);
}

Value ArrayObject::assoc(Env *env, Value needle) {
    // TODO use common logic for this (see for example rassoc and index)
    for (auto &item : *this) {
        if (!item.is_array() && !item.respond_to(env, "to_ary"_s))
            continue;

        ArrayObject *sub_array = item.to_ary(env);
        if (sub_array->is_empty())
            continue;

        if ((*sub_array)[0].send(env, "=="_s, { needle }).is_truthy())
            return sub_array;
    }

    return Value::nil();
}

Value ArrayObject::bsearch(Env *env, Block *block) {
    if (!block) {
        return send(env, "enum_for"_s, { "bsearch"_s });
    }
    auto index = bsearch_index(env, block);
    if (index.is_nil())
        return index;

    return (*this)[index.integer().to_nat_int_t()];
}

Value ArrayObject::bsearch_index(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "bsearch_index"_s });

    nat_int_t left = 0;
    nat_int_t right = m_vector.size() - 1;

    auto result = binary_search(env, left, right, [this, env, block](nat_int_t middle) -> Value {
        return block->run(env, { (*this)[middle] }, nullptr);
    });

    if (!result.present())
        return Value::nil();

    return Value::integer(result.value());
}

Value ArrayObject::rassoc(Env *env, Value needle) {
    for (auto &item : *this) {
        if (!item.is_array() && !item.respond_to(env, "to_ary"_s))
            continue;

        ArrayObject *sub_array = item.to_ary(env);
        if (sub_array->size() < 2)
            continue;

        if (sub_array->at(1).send(env, "=="_s, { needle }).is_truthy())
            return sub_array;
    }

    return Value::nil();
}

Value ArrayObject::hash(Env *env) {
    TM::RecursionGuard guard { this };
    return guard.run([&](bool is_recursive) {
        if (is_recursive)
            return Value { Value::nil() };

        HashBuilder hash {};
        auto hash_method = "hash"_s;

        for (size_t i = 0; i < size(); ++i) {
            auto item = (*this)[i];
            auto item_hash = item.send(env, hash_method);

            if (item_hash.is_nil())
                continue;

            // this allows us to return the same hash for recursive arrays:
            // a = []; a << a; a.hash == [a].hash # => true
            // a = []; a << a << a; a.hash == [a, a].hash # => true
            if (item.is_array() && size() == item.as_array()->size() && eql(env, item))
                continue;

            auto nat_int = IntegerMethods::convert_to_nat_int_t(env, item_hash);
            hash.append(nat_int);
        }

        return Value::integer(hash.digest());
    });
}

Value ArrayObject::insert(Env *env, Args &&args) {
    this->assert_not_frozen(env);

    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (args.size() == 1)
        return this;

    auto index_ptr = args[0];

    auto index = IntegerMethods::convert_to_nat_int_t(env, index_ptr);
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
        m_vector.push(Value::nil());
    }

    for (size_t i = 1; i < args.size(); i++) {
        m_vector.insert(i + size_t_index - 1, args[i]);
    }

    return this;
}

Value ArrayObject::intersection(Env *env, Value arg) {
    return intersection(env, Args { arg });
}

bool ArrayObject::include_eql(Env *env, Value arg) {
    auto eql = "eql?"_s;
    for (auto &val : *this) {
        if (object_id(arg) == object_id(val) || arg.send(env, eql, { val }).is_truthy())
            return true;
    }
    return false;
}

Value ArrayObject::intersection(Env *env, Args &&args) {
    auto *result = ArrayObject::create(m_vector);
    result->uniq_in_place(env, nullptr);

    TM::Vector<ArrayObject *> arrays;

    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];
        ArrayObject *other_array = arg.to_ary(env);

        if (!other_array->is_empty())
            arrays.push(other_array);
    }

    if (result->is_empty()) return result;
    if (arrays.size() != args.size()) return ArrayObject::create();

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

bool ArrayObject::intersects(Env *env, Value arg) {
    if (this->is_empty()) return false;

    ArrayObject *other_array = arg.to_ary(env);

    for (auto &item : *this) {
        if (other_array->include_eql(env, item)) {
            return true;
        }
    }

    return false;
}

Value ArrayObject::union_of(Env *env, Value arg) {
    if (!arg.is_array())
        env->raise("TypeError", "no implicit conversion of {} into Array", arg.klass()->inspect_module());

    auto *result = ArrayObject::create();
    auto add_value = [&result, &env](Value &val) {
        if (!result->include(env, val)) {
            result->push(val);
        }
    };

    for (auto &val : *this) {
        add_value(val);
    }

    auto *other_array = arg.as_array();
    for (auto &val : *other_array) {
        add_value(val);
    }

    return result;
}

Value ArrayObject::union_of(Env *env, Args &&args) {
    auto *result = ArrayObject::create(m_vector);

    // TODO: we probably want to make | call this instead of this way for optimization
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];
        result = result->union_of(env, arg).as_array();
    }

    return result;
}

Value ArrayObject::unshift(Env *env, Args &&args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < args.size(); i++) {
        m_vector.insert(i, args[i]);
    }
    return this;
}

Value ArrayObject::reverse(Env *env) {
    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->reverse_in_place(env);
    return copy;
}

Value ArrayObject::reverse_each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, ArrayObject::size_fn, 0);
        return send(env, "enum_for"_s, { "reverse_each"_s }, size_block);
    }

    for (size_t i = m_vector.size(); i > 0; --i) {
        Value args[] = { (*this)[i - 1] };
        block->run(env, Args(1, args), nullptr);
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

Value ArrayObject::concat(Env *env, Args &&args) {
    assert_not_frozen(env);

    const auto original_size = size();

    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this) {
            auto original = m_vector.slice(0, original_size);
            m_vector.concat(original);
        } else {
            concat(*arg.to_ary(env));
        }
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

Value ArrayObject::rindex(Env *env, Optional<Value> object, Block *block) {
    if (!block && !object) return send(env, "enum_for"_s, { "rindex"_s });
    return find_index(env, object, block, true);
}

Value ArrayObject::fetch(Env *env, Value arg_index, Optional<Value> default_value, Block *block) {
    Value value;
    auto index_val = IntegerMethods::convert_to_nat_int_t(env, arg_index);
    auto index = _resolve_index(index_val);
    if (index < 0) {
        if (block) {
            if (default_value)
                env->warn("block supersedes default value argument");

            Value args[] = { arg_index };
            value = block->run(env, Args(1, args), nullptr);
        } else if (default_value) {
            value = default_value.value();
        } else {
            env->raise("IndexError", "index {} outside of array bounds: {}...{}", index_val, -(nat_int_t)size(), size());
        }
    } else {
        value = m_vector[index];
    }

    return value;
}

Value ArrayObject::fetch_values(Env *env, Args &&args, Block *block) {
    const auto size = args.size();
    auto result = new ArrayObject { size };
    for (size_t i = 0; i < size; i++) {
        result->push(fetch(env, args[i], {}, block));
    }
    return result;
}

Value ArrayObject::find_index(Env *env, Optional<Value> object, Block *block, bool search_reverse) {
    if (object && block) env->warn("given block not used");
    assert(size() <= NAT_INT_MAX);

    auto length = static_cast<nat_int_t>(size());
    for (nat_int_t i = 0; i < length; i++) {
        nat_int_t index = search_reverse ? length - i - 1 : i;
        auto item = m_vector[index];
        if (object) {
            if (item.send(env, "=="_s, { object.value() }).is_truthy())
                return Value::integer(index);
        } else {
            Value args[] = { item };
            auto result = block->run(env, Args(1, args), nullptr);
            length = static_cast<nat_int_t>(size());
            if (result.is_truthy())
                return Value::integer(index);
        }
    }
    return Value::nil();
}

Value ArrayObject::none(Env *env, Args &&args, Block *block) {
    // FIXME: delegating to Enumerable#none? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#none? in MRI and it won't affect Array#none?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "none?"_s);
    return method_info.method()->call(env, this, std::move(args), block);
}

Value ArrayObject::one(Env *env, Args &&args, Block *block) {
    // FIXME: delegating to Enumerable#one? like this does not have the same semantics as MRI,
    // i.e. one can override Enumerable#one? in MRI and it won't affect Array#one?.
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "one?"_s);
    return method_info.method()->call(env, this, std::move(args), block);
}

Value ArrayObject::product(Env *env, Args &&args, Block *block) {
    Vector<ArrayObject *> arrays;
    arrays.push(this);
    for (size_t i = 0; i < args.size(); ++i)
        arrays.push(args[i].to_ary(env));

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

    ArrayObject *products = ArrayObject::create(number_of_combinations);
    for (size_t iteration = 0; iteration < number_of_combinations; ++iteration) {
        ArrayObject *product = ArrayObject::create(arrays.size());
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

Value ArrayObject::rotate(Env *env, Optional<Value> val) {
    ArrayObject *copy = ArrayObject::create(m_vector);
    copy->rotate_in_place(env, val);
    return copy;
}

Value ArrayObject::rotate_in_place(Env *env, Optional<Value> val) {
    assert_not_frozen(env);
    nat_int_t count = 1;
    if (val)
        count = IntegerMethods::convert_to_nat_int_t(env, val.value());

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

Value ArrayObject::slice_in_place(Env *env, Value index_obj, Optional<Value> size) {
    this->assert_not_frozen(env);

    Integer size_int;
    if (size) {
        index_obj = index_obj.to_int(env);
        size_int = size.value().to_int(env);
        IntegerMethods::assert_fixnum(env, size_int);
    }

    if (index_obj.is_integer()) {
        IntegerMethods::assert_fixnum(env, index_obj.integer());

        auto start = index_obj.integer().to_nat_int_t();

        if (!size) {
            start = _resolve_index(start);
            if (start < 0 || start >= (nat_int_t)m_vector.size())
                return Value::nil();

            Value item = (*this)[start];
            for (size_t i = start; i < this->size() - 1; i++) {
                (*this)[i] = (*this)[i + 1];
            }
            m_vector.pop();
            return item;
        }

        auto length = size_int.to_nat_int_t();

        if (length < 0)
            return Value::nil();

        if (start > (nat_int_t)m_vector.size())
            return Value::nil();

        if (start == (nat_int_t)m_vector.size())
            return ArrayObject::create();

        return _slice_in_place(start, _resolve_index(start) + length, true);
    }

    if (index_obj.is_range()) {
        RangeObject *range = index_obj.as_range();
        Value begin_obj = range->begin();
        nat_int_t start;

        if (begin_obj.is_nil()) {
            start = 0;
        } else {
            start = IntegerMethods::convert_to_nat_int_t(env, begin_obj);
        }

        Value end_obj = range->end();

        nat_int_t end;

        if (end_obj.is_nil()) {
            end = this->size();
        } else {
            end = IntegerMethods::convert_to_nat_int_t(env, end_obj);
        }

        return _slice_in_place(start, end, range->exclude_end());
    }

    if (index_obj.is_enumerator_arithmetic_sequence()) {
        auto seq = index_obj.as_enumerator_arithmetic_sequence();
        Vector<Value> result {};
        const auto step = IntegerMethods::convert_to_nat_int_t(env, seq->step());
        if (step > 0) {
            nat_int_t idx = seq->begin().is_nil() ? 0 : IntegerMethods::convert_to_nat_int_t(env, seq->begin());
            if (idx < 0) idx = this->size() + idx;
            nat_int_t end = seq->end().is_nil() ? this->size() : IntegerMethods::convert_to_nat_int_t(env, seq->end());
            if (end < 0) end = this->size() + end;
            if (seq->exclude_end()) end--;
            while (idx <= end && static_cast<size_t>(idx) < this->size()) {
                result.push(m_vector[idx]);
                idx += step;
            }
        } else {
            const nat_int_t begin = seq->end().is_nil() ? 0 : IntegerMethods::convert_to_nat_int_t(env, seq->end());
            nat_int_t idx = seq->begin().is_nil() ? this->size() : IntegerMethods::convert_to_nat_int_t(env, seq->begin());
            if (begin < 0 && idx != 0) {
                idx = -1; // break early
            } else {
                if (idx < 0) idx = this->size() + idx;
                if (seq->exclude_end()) idx--;
            }
            if (idx >= static_cast<nat_int_t>(this->size())) idx = this->size() - 1;
            while (idx >= begin && idx >= 0) {
                result.push(m_vector[idx]);
                idx += step;
            }
        }
        m_vector = std::move(result);
        return this;
    }

    return slice_in_place(env, index_obj.to_int(env), size);
}

Value ArrayObject::_slice_in_place(nat_int_t start, nat_int_t end, bool exclude_end) {
    if (start > (nat_int_t)m_vector.size())
        return Value::nil();

    if (start == (nat_int_t)m_vector.size())
        return ArrayObject::create();

    start = _resolve_index(start);

    if (start < 0)
        return Value::nil();

    if (end < 0)
        end = _resolve_index(end);

    nat_int_t length = end - start + (exclude_end ? 0 : 1);

    if (length < 0)
        length = 0;

    if (length + start > (nat_int_t)this->size())
        length = this->size() - start;

    if (length < 0)
        return Value::nil();

    ArrayObject *newArr = ArrayObject::create();
    if (length == 0) {
        return newArr;
    }

    if (start + length > (nat_int_t)this->size()) {
        length = this->size() - start;
    }

    newArr->m_vector.concat_slice(m_vector, start, length);

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
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "to_h"_s);
    return method_info.method()->call(env, this, {}, block);
}

Value ArrayObject::try_convert(Env *env, Value val) {
    auto to_ary = "to_ary"_s;

    if (val.is_array()) {
        return val;
    }

    if (!val.respond_to(env, to_ary)) {
        return Value::nil();
    }

    auto conversion = val.send(env, to_ary);

    if (conversion.is_array() || conversion.is_nil()) {
        return conversion;
    }

    auto original_item_class_name = val.klass()->inspect_module();
    auto new_item_class_name = conversion.klass()->inspect_module();
    env->raise(
        "TypeError",
        "can't convert {} to Array ({}#to_ary gives {})",
        original_item_class_name,
        original_item_class_name,
        new_item_class_name);
}

Value ArrayObject::values_at(Env *env, Args &&args) {
    TM::Vector<nat_int_t> indices;

    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];
        if (arg.is_range()) {
            auto begin_value = arg.as_range()->begin();
            auto end_value = arg.as_range()->end();
            nat_int_t begin, end;

            if (begin_value.is_nil()) {
                begin = 0;
            } else {
                begin = IntegerMethods::convert_to_nat_int_t(env, begin_value);
                if (begin < -1 * (nat_int_t)(this->size())) {
                    env->raise("RangeError", "{} out of range", arg.as_range()->inspected(env));
                }
                if (begin < 0)
                    break;
            }

            if (end_value.is_nil()) {
                end = size();
            } else {
                end = IntegerMethods::convert_to_nat_int_t(env, end_value);
                if (!arg.as_range()->exclude_end())
                    end += 1;
                if (end < 0)
                    end += size();
                if (end < begin)
                    break;
            }

            for (nat_int_t j = begin; j < end; ++j)
                indices.push(j);
        } else {
            indices.push(IntegerMethods::convert_to_nat_int_t(env, arg));
        }
    }

    auto accumulator = ArrayObject::create(indices.size());
    for (auto index : indices) {
        auto resolved_index = _resolve_index(index);
        if (resolved_index < 0 || static_cast<size_t>(resolved_index) >= m_vector.size()) {
            accumulator->push(Value::nil());
            continue;
        }
        accumulator->push((*this)[resolved_index]);
    }

    return accumulator;
}

Value ArrayObject::zip(Env *env, Args &&args, Block *block) {
    // FIXME: this is not exactly the way ruby does it
    auto Enumerable = GlobalEnv::the()->Object()->const_fetch("Enumerable"_s).as_module();
    auto method_info = Enumerable->find_method(env, "zip"_s);
    return method_info.method()->call(env, this, std::move(args), block);
}
}
