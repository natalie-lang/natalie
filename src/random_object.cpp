#include "natalie.hpp"
#include <natalie/random_object.hpp>
#include <random>

namespace Natalie {
Value RandomObject::initialize(Env *env, Value seed) {
    if (!seed) {
        m_seed = (nat_int_t)std::random_device()();
    } else {
        if (seed->is_float()) {
            seed = seed->as_float()->to_i(env);
        }

        m_seed = IntegerObject::convert_to_nat_int_t(env, seed);
    }

    if (m_generator) delete m_generator;
    this->m_generator = new std::mt19937(m_seed);
    return this;
}

Value RandomObject::bytes(Env *env, Value size) {
    assert(m_generator);

    const auto isize = size->to_int(env)->to_nat_int_t();
    if (isize < 0)
        env->raise("ArgumentError", "negative string size (or size too big)");

    const auto blocks = (static_cast<size_t>(isize) + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    nat_int_t output[blocks];
    std::uniform_int_distribution<uint32_t> random_number {};
    for (size_t i = 0; i < blocks; i++)
        output[i] = random_number(*m_generator);

    return new StringObject { reinterpret_cast<char *>(output), static_cast<size_t>(isize), EncodingObject::get(Encoding::ASCII_8BIT) };
}

Value RandomObject::rand(Env *env, Value arg) {
    if (arg) {
        if (arg->is_float()) {
            double max = arg->as_float()->to_double();
            if (max <= 0) {
                env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
            }
            return generate_random(0.0, max);
        } else if (arg->is_range()) {
            Value min = arg->as_range()->begin();
            Value max = arg->as_range()->end();
            // TODO: There can be different types of objects that respond to + and - (according to the docs)
            // I'm not sure how we should handle those though (coerce via to_int or to_f?)
            if (min->is_numeric() && max->is_numeric()) {
                if (min.send(env, ">"_s, { max })->is_true()) {
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
        }

        nat_int_t max = IntegerObject::convert_to_nat_int_t(env, arg);
        if (max <= 0) {
            env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
        }
        return generate_random(0, max - 1);
    } else {
        return generate_random(0.0, 1.0);
    }
}

}
