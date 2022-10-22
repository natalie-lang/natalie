#include "natalie.hpp"
#include "natalie/bsearch.hpp"

namespace Natalie {

RangeObject *RangeObject::create(Env *env, Value begin, Value end, bool exclude_end) {
    assert_no_bad_value(env, begin, end);
    auto range = new RangeObject(begin, end, exclude_end);
    range->freeze();
    return range;
}

void RangeObject::assert_no_bad_value(Env *env, Value begin, Value end) {
    if (!begin->is_nil() && !end->is_nil() && begin->send(env, "<=>"_s, { end })->is_nil())
        env->raise("ArgumentError", "bad value for range");
}

Value RangeObject::initialize(Env *env, Value begin, Value end, Value exclude_end_value) {
    assert_not_frozen(env);

    assert_no_bad_value(env, begin, end);

    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    freeze();

    return this;
}

template <typename Function>
Value RangeObject::iterate_over_range(Env *env, Function &&func) {
    Value item = m_begin;

    auto succ = "succ"_s;
    if (!m_begin->respond_to(env, succ))
        env->raise("TypeError", "can't iterate from {}", m_begin->klass()->inspect_str());

    if (m_begin->is_string() && m_end->is_string())
        return iterate_over_string_range(env, func);
    else if (m_begin->is_symbol() && m_end->is_symbol())
        return iterate_over_symbol_range(env, func);

    auto cmp = "<=>"_s;
    // If we exclude the end, the loop should not be entered if m_begin (item) == m_end.
    bool done = m_exclude_end ? item.send(env, "=="_s, { m_end })->is_truthy() : false;
    while (!done) {
        if (!m_end->is_nil()) {
            auto compare_result = item.send(env, cmp, { m_end })->to_int(env)->integer();
            // We are done if we reached the end element.
            done = compare_result == 0;
            // If we exclude the end we break instantly, otherwise we yield the item once again. We also break if item is bigger than end.
            if ((done && m_exclude_end) || compare_result == 1)
                break;
        }

        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(item);
        } else {
            if (Value ptr = func(item))
                return ptr;
        }

        if (!done) {
            if (!item->respond_to(env, "succ"_s))
                break;
            item = item.send(env, succ);
        }
    }

    return nullptr;
}

template <typename Function>
Value RangeObject::iterate_over_string_range(Env *env, Function &&func) {
    if (m_begin.send(env, "<=>"_s, { m_end })->equal(Value::integer(1)))
        return nullptr;

    TM::Optional<TM::String> current;
    auto end = m_end->as_string()->string();
    auto iterator = StringUptoIterator(m_begin->as_string()->string(), end, m_exclude_end);

    while ((current = iterator.next()).present()) {
        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(new StringObject { current.value() });
        } else {
            if (Value ptr = func(new StringObject { current.value() }))
                return ptr;
        }
    }

    return nullptr;
}

template <typename Function>
Value RangeObject::iterate_over_symbol_range(Env *env, Function &&func) {
    if (m_begin.send(env, "<=>"_s, { m_end })->equal(Value::integer(1)))
        return nullptr;

    TM::Optional<TM::String> current;
    auto end = m_end->as_symbol()->string();
    auto iterator = StringUptoIterator(m_begin->as_symbol()->string(), end, m_exclude_end);

    while ((current = iterator.next()).present()) {
        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(SymbolObject::intern(current.value()));
        } else {
            if (Value ptr = func(SymbolObject::intern(current.value())))
                return ptr;
        }
    }

    return nullptr;
}

Value RangeObject::to_a(Env *env) {
    if (m_end->is_nil())
        env->raise("RangeError", "cannot convert endless range to an array");

    ArrayObject *ary = new ArrayObject {};
    iterate_over_range(env, [&](Value item) {
        ary->push(item);
    });
    return ary;
}

Value RangeObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { env, this, RangeObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }

    Value break_value = iterate_over_range(env, [&](Value item) -> Value {
        Value args[] = { item };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
        return nullptr;
    });
    if (break_value) {
        return break_value;
    }

    return this;
}

Value RangeObject::first(Env *env, Value n) {
    if (m_begin->is_nil()) {
        env->raise("RangeError", "cannot get the first element of beginless range");
    }
    if (n) {
        nat_int_t count = IntegerObject::convert_to_nat_int_t(env, n);
        if (count < 0) {
            env->raise("ArgumentError", "negative array size (or size too big)");
            return nullptr;
        }

        ArrayObject *ary = new ArrayObject { (size_t)count };
        iterate_over_range(env, [&](Value item) -> Value {
            if (count == 0) return n;

            ary->push(item);
            count--;
            return nullptr;
        });

        return ary;
    } else {
        return begin();
    }
}

Value RangeObject::inspect(Env *env) {
    if (m_exclude_end) {
        if (m_end->is_nil()) {
            if (m_begin->is_nil()) {
                return new StringObject { "nil...nil" };
            } else {
                return StringObject::format("{}...", m_begin->inspect_str(env));
            }
        } else {
            if (m_begin->is_nil()) {
                return StringObject::format("...{}", m_end->inspect_str(env));
            } else {
                return StringObject::format("{}...{}", m_begin->inspect_str(env), m_end->inspect_str(env));
            }
        }
    } else {
        if (m_end->is_nil()) {
            if (m_begin->is_nil()) {
                return new StringObject { "nil..nil" };
            } else {
                return StringObject::format("{}..", m_begin->inspect_str(env));
            }
        } else {
            if (m_begin->is_nil()) {
                return StringObject::format("..{}", m_end->inspect_str(env));
            } else {
                return StringObject::format("{}..{}", m_begin->inspect_str(env), m_end->inspect_str(env));
            }
        }
    }
}

Value RangeObject::last(Env *env, Value n) {
    if (m_end->is_nil())
        env->raise("RangeError", "cannot get the last element of endless range");

    if (!n)
        return end();

    return to_a(env)->as_array()->last(env, n);
}

String RangeObject::dbg_inspect() const {
    String str;
    auto append = [&](Value v) {
        if (v.is_fast_integer()) {
            str.append(v.get_fast_integer());
        } else if (v.is_fast_double()) {
            auto f = FloatObject(v.get_fast_double());
            str.append(f.to_s());
        } else {
            auto obj = v.object_or_null();
            assert(obj);
            if (!obj->is_nil())
                str.append(obj->dbg_inspect());
        }
    };
    append(m_begin);
    str.append(m_exclude_end ? "..." : "..");
    append(m_end);
    return str;
}

Value RangeObject::to_s(Env *env) {
    auto begin = m_begin->send(env, "to_s"_s)->as_string();
    auto end = m_end->send(env, "to_s"_s)->as_string();
    return StringObject::format(m_exclude_end ? "{}...{}" : "{}..{}", begin, end);
}

bool RangeObject::eq(Env *env, Value other_value) {
    if (other_value->is_range()) {
        RangeObject *other = other_value->as_range();
        Value begin = other->begin();
        Value end = other->end();
        bool begin_equal = m_begin.send(env, "=="_s, { begin })->is_truthy();
        bool end_equal = m_end.send(env, "=="_s, { end })->is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return true;
        }
    }
    return false;
}

bool RangeObject::eql(Env *env, Value other_value) {
    if (other_value->is_range()) {
        RangeObject *other = other_value->as_range();
        Value begin = other->begin();
        Value end = other->end();
        bool begin_equal = m_begin.send(env, "eql?"_s, { begin })->is_truthy();
        bool end_equal = m_end.send(env, "eql?"_s, { end })->is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return true;
        }
    }
    return false;
}

bool RangeObject::include(Env *env, Value arg) {
    if (arg.is_fast_integer() && m_begin.is_fast_integer() && m_end.is_fast_integer()) {
        const auto begin = m_begin.get_fast_integer();
        const auto end = m_end.get_fast_integer();
        const auto i = arg.get_fast_integer();

        const auto larger_than_begin = begin <= i;
        const auto smaller_than_end = m_exclude_end ? i < end : i <= end;
        if (larger_than_begin && smaller_than_end)
            return true;
        return false;
    } else if ((m_begin->is_nil() || m_begin->is_numeric()) && (m_end->is_nil() || m_end->is_numeric())) {
        return send(env, "cover?"_s, { arg })->is_truthy();
    } else if (m_begin->is_time() || m_end->is_time()) {
        if (m_begin->is_nil() || m_begin->as_time()->cmp(env, arg)->as_integer()->lte(env, Value::integer(0))) {
            Value integer = Value::integer(m_exclude_end ? -1 : 0);
            if (m_end->is_nil() || arg->as_time()->cmp(env, m_end)->as_integer()->lte(env, integer)) {
                return true;
            }
        }
        return false;
    }

    auto compare_result = arg.send(env, "<=>"_s, { m_begin });
    if (compare_result->equal(Value::integer(-1)))
        return false;

    auto eqeq = "=="_s;
    // NATFIXME: Break out of iteration if current arg is smaller than item.
    // This means we have to implement a way to break out of `iterate_over_range`
    Value found_item = iterate_over_range(env, [&](Value item) -> Value {
        if (arg.send(env, eqeq, { item })->is_truthy())
            return item;

        return nullptr;
    });

    return !!found_item;
}

Value RangeObject::bsearch(Env *env, Block *block) {
    if (!block)
        return enum_for(env, "bsearch");

    if (m_begin->is_integer() && m_end->is_integer()) {
        nat_int_t left = m_begin->as_integer()->integer().to_nat_int_t();
        nat_int_t right = m_end->as_integer()->integer().to_nat_int_t();

        return binary_search_integer(env, left, right, block, m_exclude_end);
    } else if (m_begin->is_integer() && m_end->is_nil()) {
        nat_int_t left = m_begin->as_integer()->integer().to_nat_int_t();
        nat_int_t right = left + 1;

        // Find a right border in which we can perform the binary search.
        while (binary_search_check(env, NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { IntegerObject::create(right) }, nullptr)) != BSearchCheckResult::SMALLER) {
            right = left + (right - left) * 2;
        }

        return binary_search_integer(env, left, right, block, false);
    } else if (m_begin->is_nil() && m_end->is_integer()) {
        nat_int_t right = m_end->as_integer()->integer().to_nat_int_t();
        nat_int_t left = right - 1;

        // Find a left border in which we can perform the binary search.
        while (binary_search_check(env, NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { IntegerObject::create(left) }, nullptr)) != BSearchCheckResult::BIGGER) {
            left = right - (right - left) * 2;
        }

        return binary_search_integer(env, left, right, block, false);
    } else if (m_begin->is_numeric() || m_end->is_numeric()) {
        double left = m_begin->is_nil() ? -std::numeric_limits<double>::infinity() : m_begin->to_f(env)->to_double();
        double right = m_end->is_nil() ? std::numeric_limits<double>::infinity() : m_end->to_f(env)->to_double();

        return binary_search_float(env, left, right, block, m_exclude_end);
    } else {
        env->raise("TypeError", "can't do binary search for {}", m_begin->klass()->inspect_str());
    }
}

Value RangeObject::step(Env *env, Value n, Block *block) {
    if (!n)
        n = NilObject::the();

    if (!n->is_numeric() && !n->is_nil())
        n = n->to_int(env);

    if (n.send(env, "=="_s, { Value::integer(0) })->is_true())
        env->raise("ArgumentError", "step can't be 0");

    if (m_begin->is_numeric() || m_end->is_numeric()) {
        auto begin = m_begin;
        auto end = m_end;
        auto enumerator = Enumerator::ArithmeticSequenceObject::from_range(env, env->current_method()->name(), m_begin, m_end, n, m_exclude_end);

        if (block) {
            if (enumerator->step().send(env, "<"_s, { Value::integer(0) })->is_true())
                env->raise("ArgumentError", "step can't be negative");

            enumerator->send(env, "each"_s, {}, block);
        } else {
            return enumerator;
        }
    } else {
        if (n->is_nil())
            n = Value::integer(1);

        if (!block)
            return enum_for(env, "step", { n });

        if (n.send(env, "<"_s, { Value::integer(0) })->is_true())
            env->raise("ArgumentError", "step can't be negative");

        // This error is weird...
        //   - It only appears for floats (not for rational for example)
        //   - Class names are written in lower case?
        if (n->is_float())
            env->raise("TypeError", "no implicit conversion to float from {}", m_begin->klass()->inspect_str().lowercase());

        auto step = n->to_int(env)->integer();

        Integer index = 0;
        iterate_over_range(env, [env, block, &index, step](Value item) -> Value {
            if (index % step == 0)
                NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { item }, nullptr);

            index += 1;
            return nullptr;
        });
    }

    return this;
}
}
