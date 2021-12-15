#pragma once

#include <utility>

#include "natalie/class_object.hpp"
#include "natalie/float_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object.hpp"
#include <random>

namespace Natalie {

using namespace TM;

class RandomObject : public Object {
public:
    RandomObject(ClassObject *klass)
        : Object { Object::Type::Random, klass } { }

    RandomObject()
        : Object { Object::Type::Random, GlobalEnv::the()->Random() } { }

    ~RandomObject() { delete m_generator; }

    Value initialize(Env *, Value);

    Value rand(Env *, Value);
    Value seed() const { return Value::integer(m_seed); }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Random %p seed=%lld>", this, m_seed);
    }

private:
    nat_int_t m_seed;
    std::mt19937 *m_generator { nullptr };

    Value generate_random(double min, double max) {
        assert(m_generator);
        std::uniform_real_distribution<double> random_number(min, max);
        return new FloatObject { random_number(*m_generator) };
    }

    Value generate_random(nat_int_t min, nat_int_t max) {
        assert(m_generator);
        std::uniform_int_distribution<nat_int_t> random_number(min, max);
        return Value::integer(random_number(*m_generator));
    }
};

}
