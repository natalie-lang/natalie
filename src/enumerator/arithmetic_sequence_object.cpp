#include "natalie/enumerator/arithmetic_sequence_object.hpp"
#include "natalie.hpp"

namespace Natalie::Enumerator {
Integer ArithmeticSequenceObject::calculate_step_count(Env *env) {
    auto n = m_end->send(env, "-"_s, { m_begin })->send(env, "/"_s, { m_step });

    Integer step_count;
    if (n->is_integer()) {
        step_count = n->as_integer()->integer();
    } else {
        step_count = n->send(env, "+"_s, { n->send(env, "*"_s, { new FloatObject { std::numeric_limits<double>::epsilon() } }) })->send(env, "floor"_s)->as_integer()->integer();
    }

    if (exclude_end() && step_count > 0)
        step_count -= 1;

    return step_count;
}

bool ArithmeticSequenceObject::eq(Env *env, Value other) {
    if (!other->is_enumerator_arithmetic_sequence())
        return false;

    ArithmeticSequenceObject *other_sequence = other->as_enumerator_arithmetic_sequence();
    return m_begin->send(env, "=="_s, { other_sequence->begin() })->is_truthy() && m_end->send(env, "=="_s, { other_sequence->end() })->is_truthy() && m_step->send(env, "=="_s, { other_sequence->step() })->is_truthy() && m_exclude_end == other_sequence->exclude_end();
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
    add(m_step);

    if (m_exclude_end)
        add(TrueObject::the());
    else
        add(FalseObject::the());

    return IntegerObject::create(hash_builder.digest());
}

Value ArithmeticSequenceObject::last(Env *env) {
    if (m_exclude_end) {
        auto steps = step_count(env);
        return m_begin->send(env, "+"_s, { IntegerObject::create(steps)->send(env, "*"_s, { m_step }) });
    } else {
        return m_end;
    }
}
};
