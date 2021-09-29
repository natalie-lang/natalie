#pragma once

#include <utility>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/value.hpp"
#include <random>

namespace Natalie {

using namespace TM;

class RandomValue : public Value {
public:
    RandomValue(ClassValue *klass)
        : Value { Value::Type::Random, klass } { }

    RandomValue()
        : Value { Value::Type::Random, GlobalEnv::the()->Random() } { }

    ~RandomValue() { delete m_generator; }

    ValuePtr initialize(Env *, ValuePtr);

    ValuePtr rand(Env *, ValuePtr);
    ValuePtr seed() { return new IntegerValue { m_seed }; }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Random %p seed=%lld>", this, m_seed);
    }

private:
    nat_int_t m_seed;
    std::mt19937 *m_generator { nullptr };

    ValuePtr generate_random(double min, double max) {
        assert(m_generator);
        std::uniform_real_distribution<double> random_number(min, max);
        return new FloatValue { random_number(*m_generator) };
    }

    ValuePtr generate_random(nat_int_t min, nat_int_t max) {
        assert(m_generator);
        std::uniform_int_distribution<nat_int_t> random_number(min, max);
        return new IntegerValue { random_number(*m_generator) };
    }
};

}
