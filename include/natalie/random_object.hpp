#pragma once

#include <utility>

#include "natalie/class_object.hpp"
#include "natalie/float_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/object.hpp"
#include <random>

#ifdef __linux__
#include <sys/random.h>
#endif

namespace Natalie {

using namespace TM;

class RandomObject : public Object {
public:
    RandomObject(ClassObject *klass)
        : Object { Object::Type::Random, klass } { }

    RandomObject()
        : Object { Object::Type::Random, GlobalEnv::the()->Random() } { }

    ~RandomObject() { delete m_generator; }

    Value initialize(Env *, Optional<Value> = {});

    Value bytes(Env *, Value);
    Value rand(Env *, Optional<Value>);
    Value seed() const { return Value::integer(m_seed); }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<Random {h} seed={}>", this, m_seed);
    }

    static Value new_seed(Env *env) {
        return Value::integer(std::random_device()());
    }

    static Value srand(Env *env, Optional<Value> seed_arg) {
        auto seed = seed_arg.value_or([&env]() { return new_seed(env); });
        auto default_random = GlobalEnv::the()->Random()->const_fetch("DEFAULT"_s).as_random();
        auto old_seed = default_random->seed();
        auto new_seed = IntegerMethods::convert_to_native_type<nat_int_t>(env, seed);
        default_random->m_seed = new_seed;
        delete default_random->m_generator;
        default_random->m_generator = new std::mt19937(new_seed);
        return old_seed;
    }

    static Value urandom(Env *env, Value size) {
        auto integer = size.integer();
        if (integer.is_negative())
            env->raise("ArgumentError", "negative string size (or size too big)");
        if (integer.is_zero())
            return new StringObject { "", Encoding::ASCII_8BIT };

        size_t length = (size_t)integer.to_nat_int_t();
        char buffer[length];
#ifdef __linux__
        auto result = getrandom(buffer, length, 0);
        if (result == -1)
            env->raise_errno();
#else
        arc4random_buf(buffer, length);
#endif
        return new StringObject { buffer, length, Encoding::ASCII_8BIT };
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
