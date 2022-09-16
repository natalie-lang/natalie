#include "natalie/enumerator/arithmetic_sequence_object.hpp"
#include "natalie.hpp"

namespace Natalie::Enumerator {
ArithmeticSequenceObject::ArithmeticSequenceObject(Origin origin, Value begin, Value end, Value step, bool exclude_end)
    : ArithmeticSequenceObject {} {
    m_origin = origin;
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end;
    m_step = step;
}

Integer ArithmeticSequenceObject::calculate_step_count(Env *env) {
    if (m_end->send(env, "infinite?"_s)->is_truthy())
        return 0;

    auto _step = step();
    auto _begin = m_begin;
    auto _end = m_end;

    if (_step->is_float() || _begin->is_float() || _end->is_float()) {
        _step = _step->to_f(env);
        _begin = _begin->to_f(env);
        _end = _end->to_f(env);
    }

    auto n = _end->send(env, "-"_s, { _begin })->send(env, "/"_s, { _step->to_f(env) });

    if (n->send(env, "=="_s, { n->send(env, "floor"_s) })->is_truthy())
        n = n->to_int(env);

    Integer step_count;
    if (n->is_integer()) {
        step_count = n->as_integer()->integer();

        if (!exclude_end())
            step_count += 1;
    } else {
        step_count = n->send(env, "+"_s, { n->send(env, "*"_s, { new FloatObject { std::numeric_limits<double>::epsilon() } }) })->send(env, "floor"_s)->as_integer()->integer() + 1;
    }

    return step_count;
}

Value ArithmeticSequenceObject::each(Env *env, Block *block) {
    if (!block)
        return this;

    auto _step = step();
    auto _begin = m_begin;
    auto _end = m_end;

    // This should be above the float conversion because String#to_f exists but should
    // raise an ArgumentError here
    auto ascending = _step->send(env, ">"_s, { Value::integer(0) })->is_truthy();
    auto cmp = ascending ? ">"_s : "<"_s;

    if (_step->is_float() || _begin->is_float() || _end->is_float()) {
        _step = _step->to_f(env);
        _begin = _begin->to_f(env);
        _end = _end->to_f(env);
    }

    if (_step->send(env, "infinite?"_s)->is_truthy()) {
        if (_begin->send(env, cmp, { _end })->is_falsey())
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { _begin }, nullptr);
        return this;
    }

    if (_begin->send(env, "infinite?"_s)->is_truthy() && _begin->send(env, cmp, { Value::integer(0) })->is_truthy())
        return this;

    auto infinite = _end->send(env, "infinite?"_s)->is_truthy();
    auto steps = step_count(env);

    // Check that infinity goes in step's direction
    infinite = infinite && _end->send(env, cmp, { Value::integer(0) })->is_truthy();

    for (Integer i = 0; infinite || i < steps; ++i) {
        auto value = _step->send(env, "*"_s, { IntegerObject::create(i) })->send(env, "+"_s, { _begin });

        // Ensure that we do not yield a number that exceeds `_end`
        if (value->send(env, cmp, { _end })->is_truthy())
            value = _end;

        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { value }, nullptr);
    }

    return this;
}

bool ArithmeticSequenceObject::eq(Env *env, Value other) {
    if (!other->is_enumerator_arithmetic_sequence())
        return false;

    ArithmeticSequenceObject *other_sequence = other->as_enumerator_arithmetic_sequence();
    return hash(env)->equal(other_sequence->hash(env));
}

Value ArithmeticSequenceObject::hash(Env *env) {
    HashBuilder hash_builder {};
    auto hash_method = "hash"_s;

    auto add = [&hash_builder, &hash_method, env](Value value) {
        auto hash = value->send(env, hash_method);

        if (hash->is_nil())
            return;

        auto nat_int = IntegerObject::convert_to_nat_int_t(env, hash);
        hash_builder.append(nat_int);
    };

    add(m_begin);
    add(m_end);
    add(step());

    if (m_exclude_end)
        add(TrueObject::the());
    else
        add(FalseObject::the());

    return IntegerObject::create(hash_builder.digest());
}

Value ArithmeticSequenceObject::inspect(Env *env) {
    switch (m_origin) {
    case Origin::Range: {
        auto range_inspect = RangeObject(m_begin, m_end, m_exclude_end).inspect_str(env);
        auto string = StringObject::format("(({}).step", range_inspect);

        if (has_step()) {
            string->append_char('(');
            string->append(m_step);
            string->append_char(')');
        }

        string->append_char(')');

        return string;
    }
    case Origin::Numeric: {
        auto string = StringObject::format("({}.step({}", m_begin, m_end);

        if (has_step()) {
            string->append(", ");
            string->append(m_step);
        }

        string->append("))");

        return string;
    }
    }
    return nullptr;
}

Value ArithmeticSequenceObject::last(Env *env, Value n) {
    auto _end = maybe_to_f(env, m_end);
    auto steps = step_count(env);

    if (n) {
        auto n_as_int = n->to_int(env);
        Integer count = n_as_int->integer();

        if (count < 0)
            env->raise("ArgumentError", "negative array size");

        if (n_as_int->integer() > steps)
            count = steps;

        n_as_int->assert_fixnum(env);

        auto array = new ArrayObject { (size_t)count.to_nat_int_t() };

        auto _begin = maybe_to_f(env, m_begin);
        auto last = _begin->send(env, "+"_s, { step().send(env, "*"_s, { IntegerObject::create(steps) }) });
        if (last.send(env, ">="_s, { _end })->is_truthy())
            last = last->send(env, "-"_s, { step() });

        auto begin = last->send(env, "-"_s, { step().send(env, "*"_s, { IntegerObject::create(count) }) });

        for (; count > 0; --count) {
            begin = begin->send(env, "+"_s, { step() });
            array->push(begin);
        }

        return array;
    } else {
        auto last = m_begin->send(env, "+"_s, { step()->send(env, "*"_s, { IntegerObject::create(steps - 1) }) });
        if (last->send(env, ">"_s, { m_end })->is_truthy())
            last = last->send(env, "-"_s, { step() });
        return last;
    }
}

Value ArithmeticSequenceObject::maybe_to_f(Env *env, Value v) {
    if (m_begin->is_float() || m_end->is_float())
        return v->to_f(env);
    return v;
}

Value ArithmeticSequenceObject::size(Env *env) {
    if (m_end->send(env, "infinite?"_s)->is_truthy())
        return FloatObject::positive_infinity(env);

    return IntegerObject::create(step_count(env));
}
};
