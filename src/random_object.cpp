#include "natalie.hpp"
#include <natalie/random_object.hpp>
#include <random>

namespace Natalie {
Value RandomObject::initialize(Env *env, Optional<Value> seed_arg) {
    if (!seed_arg) {
        m_seed = (nat_int_t)std::random_device()();
    } else {
        auto seed = seed_arg.value();
        if (seed.is_float()) {
            seed = seed.as_float()->to_i(env);
        }
        m_seed = IntegerMethods::convert_to_nat_int_t(env, seed);
    }

    if (m_generator) delete m_generator;
    this->m_generator = new std::mt19937(m_seed);
    return this;
}

Value RandomObject::bytes(Env *env, Value size) {
    assert(m_generator);

    const auto isize = size.to_int(env).to_nat_int_t();
    if (isize < 0)
        env->raise("ArgumentError", "negative string size (or size too big)");

    const auto blocks = (static_cast<size_t>(isize) + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    nat_int_t output[blocks];
    std::uniform_int_distribution<uint32_t> random_number {};
    for (size_t i = 0; i < blocks; i++)
        output[i] = random_number(*m_generator);

    return new StringObject { reinterpret_cast<char *>(output), static_cast<size_t>(isize), Encoding::ASCII_8BIT };
}

Value RandomObject::rand(Env *env, Optional<Value> max_arg) {
    if (max_arg) {
        auto arg = max_arg.value();
        if (arg.is_float()) {
            double max = arg.as_float()->to_double();
            if (max <= 0) {
                env->raise("ArgumentError", "invalid argument - {}", arg.inspect_str(env));
            }
            return generate_random(0.0, max);
        } else if (arg.is_range()) {
            Value min = arg.as_range()->begin();
            Value max = arg.as_range()->end();
            // TODO: There can be different types of objects that respond to + and - (according to the docs)
            // I'm not sure how we should handle those though (coerce via to_int or to_f?)
            if (min.is_numeric() && max.is_numeric()) {
                if (min.send(env, ">"_s, { max }).is_true()) {
                    env->raise("ArgumentError", "invalid argument - {}", arg.inspect_str(env));
                }

                if (min.is_float() || max.is_float()) {
                    double min_rand, max_rand;
                    if (min.is_float()) {
                        min_rand = min.as_float()->to_double();
                    } else {
                        min_rand = static_cast<double>(IntegerMethods::convert_to_native_type<nat_int_t>(env, min));
                    }

                    if (max.is_float()) {
                        max_rand = max.as_float()->to_double();
                    } else {
                        max_rand = static_cast<double>(IntegerMethods::convert_to_native_type<nat_int_t>(env, max));
                    }

                    return generate_random(min_rand, max_rand);
                } else {
                    auto min_rand = IntegerMethods::convert_to_native_type<nat_int_t>(env, min);
                    auto max_rand = IntegerMethods::convert_to_native_type<nat_int_t>(env, max);

                    if (arg.as_range()->exclude_end()) {
                        max_rand -= 1;
                    }

                    return generate_random(min_rand, max_rand);
                }
            }
            env->raise("ArgumentError", "bad value for range");
        } else if (arg.is_integer() && arg.integer().is_bignum()) {
            if (arg.integer().is_negative())
                env->raise("ArgumentError", "invalid argument - {}", arg.to_s(env)->string());
            auto fraction = generate_random(0.0, 1.0);
            return arg.send(env, "*"_s, { fraction }).send(env, "to_i"_s);
        }

        if (arg.is_nil())
            env->raise("ArgumentError", "invalid argument - {}", arg.to_s(env)->string());

        nat_int_t max = IntegerMethods::convert_to_nat_int_t(env, arg);
        if (max <= 0) {
            env->raise("ArgumentError", "invalid argument - {}", arg.inspect_str(env));
        }
        return generate_random(0, max - 1);
    } else {
        return generate_random(0.0, 1.0);
    }
}

}
