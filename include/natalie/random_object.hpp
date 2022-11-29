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
        snprintf(buf, len, "<Random %p seed=%lld>", this, (long long)m_seed);
    }

    static Value new_seed(Env *env) {
        return Value::integer(std::random_device()());
    }

    static Value srand(Env *env, Value seed) {
        if (!seed || seed->is_nil())
            seed = new_seed(env);
        seed->assert_type(env, Type::Integer, "Integer");
        auto default_random = GlobalEnv::the()->Random()->const_fetch("DEFAULT"_s)->as_random();
        auto old_seed = default_random->seed();
        auto new_seed = seed->as_integer()->to_nat_int_t();
        default_random->m_seed = new_seed;
        default_random->m_generator = new std::mt19937(new_seed);
        return old_seed;
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
