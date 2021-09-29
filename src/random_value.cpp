#include "natalie.hpp"
#include <natalie/random_value.hpp>
#include <random>

namespace Natalie {
ValuePtr RandomValue::initialize(Env *env, ValuePtr seed) {
    if (!seed) {
        m_seed = (nat_int_t)std::random_device()();
    } else {
        if (seed->is_float()) {
            seed = seed->as_float()->to_i(env);
        }
        if (!seed->is_integer() && seed->respond_to(env, SymbolValue::intern("to_int"))) {
            seed = seed.send(env, SymbolValue::intern("to_int"));
        }

        seed->assert_type(env, Type::Integer, "Integer");

        m_seed = seed->as_integer()->to_nat_int_t();
    }

    if (m_generator) delete m_generator;
    this->m_generator = new std::mt19937(m_seed);
    return this;
}

ValuePtr RandomValue::rand(Env *env, ValuePtr arg) {
    if (arg) {
        if (arg->is_float()) {
            double max = arg->as_float()->to_double();
            if (max <= 0) {
                env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
            }
            return generate_random(0.0, max);
        } else if (arg->is_range()) {
            ValuePtr min = arg->as_range()->begin();
            ValuePtr max = arg->as_range()->end();
            // TODO: There can be different types of objects that respond to + and - (according to the docs)
            // I'm not sure how we should handle those though (coerce via to_int or to_f?)
            if (min->is_numeric() && max->is_numeric()) {
                if (min.send(env, SymbolValue::intern(">"), { max })->is_true()) {
                    env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
                }

                if (min->is_float() || max->is_float()) {
                    double min_rand, max_rand;
                    if (min->is_float()) {
                        min_rand = min->as_float()->to_double();
                    } else {
                        min_rand = (double)min->as_integer()->to_nat_int_t();
                    }

                    if (max->is_float()) {
                        max_rand = max->as_float()->to_double();
                    } else {
                        max_rand = (double)max->as_integer()->to_nat_int_t();
                    }

                    return generate_random(min_rand, max_rand);
                } else {
                    nat_int_t min_rand = min->as_integer()->to_nat_int_t();
                    nat_int_t max_rand = max->as_integer()->to_nat_int_t();

                    if (arg->as_range()->exclude_end()) {
                        max_rand -= 1;
                    }

                    return generate_random(min_rand, max_rand);
                }
            }
            env->raise("ArgumentError", "bad value for range");
        } else if (!arg->is_integer() && arg->respond_to(env, SymbolValue::intern("to_int"))) {
            arg = arg->send(env, SymbolValue::intern("to_int"));
        }
        arg->assert_type(env, Type::Integer, "Integer");

        nat_int_t max = arg->as_integer()->to_nat_int_t();
        if (max <= 0) {
            env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
        }
        return generate_random(0, max - 1);
    } else {
        return generate_random(0.0, 1.0);
    }
}

}
