#include "natalie.hpp"
#include "natalie/bsearch.hpp"
#include "natalie/integer_methods.hpp"

namespace Natalie {

RangeObject *RangeObject::create(Env *env, Value begin, Value end, bool exclude_end) {
    assert_no_bad_value(env, begin, end);
    auto range = new RangeObject(begin, end, exclude_end);
    range->freeze();
    return range;
}

void RangeObject::assert_no_bad_value(Env *env, Value begin, Value end) {
    if (!begin.is_nil() && !end.is_nil() && begin.send(env, "<=>"_s, { end }).is_nil())
        env->raise("ArgumentError", "bad value for range");
}

Value RangeObject::initialize(Env *env, Value begin, Value end, Optional<Value> exclude_end_value) {
    assert_not_frozen(env);

    assert_no_bad_value(env, begin, end);

    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value.value().is_truthy();
    freeze();

    return this;
}

template <typename Function>
Optional<Value> RangeObject::iterate_over_range(Env *env, Function &&func) {
    if (m_begin.is_integer())
        return iterate_over_integer_range(env, func);

    auto succ = "succ"_s;
    if (!m_begin.respond_to(env, succ))
        env->raise("TypeError", "can't iterate from {}", m_begin.klass()->inspect_module());

    if (m_begin.is_string() && m_end.is_string())
        return iterate_over_string_range(env, func);
    else if (m_begin.is_symbol() && m_end.is_symbol())
        return iterate_over_symbol_range(env, func);

    Value item = m_begin;

    auto cmp = "<=>"_s;
    // If we exclude the end, the loop should not be entered if m_begin (item) == m_end.
    bool done = m_exclude_end ? item.send(env, "=="_s, { m_end }).is_truthy() : false;
    while (!done) {
        if (!m_end.is_nil()) {
            auto compare_result = item.send(env, cmp, { m_end }).to_int(env);
            // We are done if we reached the end element.
            done = compare_result == 0;
            // If we exclude the end we break instantly, otherwise we yield the item once again. We also break if item is bigger than end.
            if ((done && m_exclude_end) || compare_result == 1)
                break;
        }

        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(item);
        } else {
            Optional<Value> result = func(item);
            if (result)
                return result;
        }

        if (!done) {
            if (!item.respond_to(env, "succ"_s))
                break;
            item = item.send(env, succ);
        }
    }

    return {};
}

template <typename Function>
Optional<Value> RangeObject::iterate_over_integer_range(Env *env, Function &&func) {
    auto end = m_end;
    if (end.is_float()) {
        if (end.as_float()->is_infinity())
            end = Value::nil();
        else
            end = end.to_int(env);
    }

    if (!end.is_nil()) {
        assert(end.is_integer());
        if (!m_exclude_end)
            end = end.integer() + Integer(1);
        for (auto i = m_begin.integer(); i < end.integer(); ++i) {
            if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
                func(i);
            } else {
                Optional<Value> result = func(i);
                if (result)
                    return result;
            }
        }
    } else {
        for (auto i = m_begin.integer();; ++i) {
            if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
                func(i);
            } else {
                Optional<Value> result = func(i);
                if (result)
                    return result;
            }
        }
    }
    return {};
}

template <typename Function>
Optional<Value> RangeObject::iterate_over_string_range(Env *env, Function &&func) {
    if (Object::equal(m_begin.send(env, "<=>"_s, { m_end }), Value::integer(1)))
        return {};

    TM::Optional<TM::String> current;
    auto end = m_end.as_string()->string();
    auto iterator = StringUptoIterator(m_begin.as_string()->string(), end, m_exclude_end);

    while ((current = iterator.next()).present()) {
        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(StringObject::create(current.value()));
        } else {
            Optional<Value> result = func(StringObject::create(current.value()));
            if (result)
                return result;
        }
    }

    return {};
}

template <typename Function>
Optional<Value> RangeObject::iterate_over_symbol_range(Env *env, Function &&func) {
    if (Object::equal(m_begin.send(env, "<=>"_s, { m_end }), Value::integer(1)))
        return {};

    TM::Optional<TM::String> current;
    auto end = m_end.as_symbol()->string();
    auto iterator = StringUptoIterator(m_begin.as_symbol()->string(), end, m_exclude_end);

    while ((current = iterator.next()).present()) {
        if constexpr (std::is_void_v<std::invoke_result_t<Function, Value>>) {
            func(SymbolObject::intern(current.value()));
        } else {
            Optional<Value> result = func(SymbolObject::intern(current.value()));
            if (result)
                return result;
        }
    }

    return {};
}

Value RangeObject::to_a(Env *env) {
    if (m_end.is_nil())
        env->raise("RangeError", "cannot convert endless range to an array");

    ArrayObject *ary = ArrayObject::create();
    iterate_over_range(env, [&](Value item) {
        ary->push(item);
    });
    return ary;
}

Value RangeObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = Block::create(*env, this, RangeObject::size_fn, 0);
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }

    auto break_value = iterate_over_range(env, [&](Value item) -> Optional<Value> {
        Value args[] = { item };
        block->run(env, Args(1, args), nullptr);
        return {};
    });
    if (break_value)
        return break_value.value();

    return this;
}

Value RangeObject::first(Env *env, Optional<Value> n) {
    if (m_begin.is_nil()) {
        env->raise("RangeError", "cannot get the first element of beginless range");
    }
    if (n) {
        nat_int_t count = IntegerMethods::convert_to_nat_int_t(env, n.value());
        if (count < 0) {
            env->raise("ArgumentError", "negative array size (or size too big)");
        }

        ArrayObject *ary = ArrayObject::create((size_t)count);
        iterate_over_range(env, [&](Value item) -> Optional<Value> {
            if (count == 0) return n.value();

            ary->push(item);
            count--;
            return {};
        });

        return ary;
    } else {
        return begin();
    }
}

Value RangeObject::inspect(Env *env) {
    if (m_exclude_end) {
        if (m_end.is_nil()) {
            if (m_begin.is_nil()) {
                return StringObject::create("nil...nil");
            } else {
                return StringObject::format("{}...", m_begin.inspected(env));
            }
        } else {
            if (m_begin.is_nil()) {
                return StringObject::format("...{}", m_end.inspected(env));
            } else {
                return StringObject::format("{}...{}", m_begin.inspected(env), m_end.inspected(env));
            }
        }
    } else {
        if (m_end.is_nil()) {
            if (m_begin.is_nil()) {
                return StringObject::create("nil..nil");
            } else {
                return StringObject::format("{}..", m_begin.inspected(env));
            }
        } else {
            if (m_begin.is_nil()) {
                return StringObject::format("..{}", m_end.inspected(env));
            } else {
                return StringObject::format("{}..{}", m_begin.inspected(env), m_end.inspected(env));
            }
        }
    }
}

Value RangeObject::last(Env *env, Optional<Value> n) {
    if (m_end.is_nil())
        env->raise("RangeError", "cannot get the last element of endless range");

    if (!n)
        return end();

    return to_a(env).as_array()->last(env, n);
}

String RangeObject::dbg_inspect(int indent) const {
    auto dots = m_exclude_end ? "..." : "..";
    return String::format("<RangeObject {h} {}{}{}>", this, m_begin.dbg_inspect(indent), dots, m_end.dbg_inspect());
}

Value RangeObject::to_s(Env *env) {
    auto begin = m_begin.send(env, "to_s"_s).as_string();
    auto end = m_end.send(env, "to_s"_s).as_string();
    return StringObject::format(m_exclude_end ? "{}...{}" : "{}..{}", begin, end);
}

bool RangeObject::eq(Env *env, Value other_value) {
    if (other_value.is_range()) {
        RangeObject *other = other_value.as_range();
        Value begin = other->begin();
        Value end = other->end();
        bool begin_equal = m_begin.send(env, "=="_s, { begin }).is_truthy();
        bool end_equal = m_end.send(env, "=="_s, { end }).is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return true;
        }
    }
    return false;
}

bool RangeObject::eql(Env *env, Value other_value) {
    if (other_value.is_range()) {
        RangeObject *other = other_value.as_range();
        Value begin = other->begin();
        Value end = other->end();
        bool begin_equal = m_begin.send(env, "eql?"_s, { begin }).is_truthy();
        bool end_equal = m_end.send(env, "eql?"_s, { end }).is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return true;
        }
    }
    return false;
}

bool RangeObject::include(Env *env, Value arg) {
    if (arg.is_integer() && m_begin.is_integer() && m_end.is_integer()) {
        const auto begin = m_begin.integer().to_nat_int_t();
        const auto end = m_end.integer().to_nat_int_t();
        const auto i = arg.integer().to_nat_int_t();

        const auto larger_than_begin = begin <= i;
        const auto smaller_than_end = m_exclude_end ? i < end : i <= end;
        if (larger_than_begin && smaller_than_end)
            return true;
        return false;
    } else if ((m_begin.is_nil() || m_begin.is_numeric()) && (m_end.is_nil() || m_end.is_numeric())) {
        return send(env, "cover?"_s, { arg }).is_truthy();
    } else if (m_begin.is_time() || m_end.is_time()) {
        if (m_begin.is_nil() || m_begin.as_time()->cmp(env, arg).integer() <= 0) {
            int end = m_exclude_end ? -1 : 0;
            if (m_end.is_nil() || arg.as_time()->cmp(env, m_end).integer() <= end)
                return true;
        }
        return false;
    }

    auto compare_result = arg.send(env, "<=>"_s, { m_begin });
    if (Object::equal(compare_result, Value::integer(-1)))
        return false;

    auto eqeq = "=="_s;
    // NATFIXME: Break out of iteration if current arg is smaller than item.
    // This means we have to implement a way to break out of `iterate_over_range`
    auto found_item = iterate_over_range(env, [&](Value item) -> Optional<Value> {
        if (arg.send(env, eqeq, { item }).is_truthy())
            return item;

        return {};
    });

    return found_item.present();
}

Value RangeObject::bsearch(Env *env, Block *block) {
    if ((!m_begin.is_numeric() && !m_begin.is_nil()) || (!m_end.is_numeric() && !m_end.is_nil()))
        env->raise("TypeError", "can't do binary search for {}", m_begin.klass()->inspect_module());

    if (!block)
        return enum_for(env, "bsearch");

    if (m_begin.is_integer() && m_end.is_integer()) {
        auto left = m_begin.integer().to_nat_int_t();
        auto right = m_end.integer().to_nat_int_t();

        return binary_search_integer(env, left, right, block, m_exclude_end);
    } else if (m_begin.is_integer() && m_end.is_nil()) {
        auto left = m_begin.integer().to_nat_int_t();
        auto right = left + 1;

        // Find a right border in which we can perform the binary search.
        while (binary_search_check(env, block->run(env, { Value::integer(right) }, nullptr)) != BSearchCheckResult::SMALLER) {
            right = left + (right - left) * 2;
        }

        return binary_search_integer(env, left, right, block, false);
    } else if (m_begin.is_nil() && m_end.is_integer()) {
        auto right = m_end.integer().to_nat_int_t();
        auto left = right - 1;

        // Find a left border in which we can perform the binary search.
        while (binary_search_check(env, block->run(env, { Value::integer(left) }, nullptr)) != BSearchCheckResult::BIGGER) {
            left = right - (right - left) * 2;
        }

        return binary_search_integer(env, left, right, block, false);
    } else if (m_begin.is_numeric() || m_end.is_numeric()) {
        double left = m_begin.is_nil() ? -std::numeric_limits<double>::infinity() : m_begin.to_f(env)->to_double();
        double right = m_end.is_nil() ? std::numeric_limits<double>::infinity() : m_end.to_f(env)->to_double();

        return binary_search_float(env, left, right, block, m_exclude_end);
    }
    NAT_UNREACHABLE();
}

Value RangeObject::step(Env *env, Optional<Value> n_arg, Block *block) {
    auto n = n_arg.value_or(Value::nil());

    if (!n.is_numeric() && !n.is_nil()) {
        static const auto coerce_sym = "coerce"_s;
        if (!n.respond_to(env, coerce_sym))
            env->raise("TypeError", "no implicit conversion of {} into Integer", n.klass()->inspect_module());
        n = n.send(env, coerce_sym, { Value::integer(0) }).as_array_or_raise(env)->last();
    }

    if (m_begin.is_numeric() || m_end.is_numeric()) {
        if (n.send(env, "=="_s, { Value::integer(0) }).is_true())
            env->raise("ArgumentError", "step can't be 0");

        auto begin = m_begin;
        auto end = m_end;
        auto enumerator = Enumerator::ArithmeticSequenceObject::from_range(env, env->current_method()->name(), m_begin, m_end, n, m_exclude_end);

        if (block) {
            if (begin.is_numeric() && end.is_numeric() && begin.send(env, "<="_s, { end }).is_true() && enumerator->step().send(env, "<"_s, { Value::integer(0) }).is_true())
                return duplicate(env);

            enumerator->send(env, "each"_s, {}, block);
        } else {
            return enumerator;
        }
    } else {
        if (n.is_nil())
            n = Value::integer(1);

        if (!block)
            return enum_for(env, "step", { n });

        if (n.send(env, "<"_s, { Value::integer(0) }).is_true())
            env->raise("ArgumentError", "step can't be negative");

        // This error is weird...
        //   - It only appears for floats (not for rational for example)
        //   - Class names are written in lower case?
        if (n.is_float())
            env->raise("TypeError", "no implicit conversion to float from {}", m_begin.klass()->inspect_module().lowercase());

        auto step = n.to_int(env);

        Integer index = 0;
        iterate_over_range(env, [env, block, &index, step](Value item) -> Optional<Value> {
            if (index % step == 0)
                block->run(env, { item }, nullptr);

            index += 1;
            return {};
        });
    }

    return this;
}
}
