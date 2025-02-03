#include "natalie.hpp"
#include <random>

using namespace Natalie;

static Value generate_random(double min, double max) {
    std::random_device rd;
    std::uniform_real_distribution<double> random_number(min, max);
    return new FloatObject { random_number(rd) };
}

static Value generate_random(nat_int_t min, nat_int_t max) {
    std::random_device rd;
    std::uniform_int_distribution<nat_int_t> random_number(min, max);
    return Value::integer(random_number(rd));
}

Value SecureRandom_random_number(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto arg = args.at(0, NilObject::the());
    if (arg.is_nil()) {
        return generate_random(0.0, 1.0);
    } else {
        if (arg.is_float()) {
            double max = arg->as_float()->to_double();
            if (max <= 0)
                max = 1.0;
            return generate_random(0.0, max);
        } else if (arg.is_range()) {
            Value min = arg->as_range()->begin();
            Value max = arg->as_range()->end();
            // TODO: There can be different types of objects that respond to + and - (according to the docs)
            // I'm not sure how we should handle those though (coerce via to_int or to_f?)
            if (min.is_numeric() && max.is_numeric()) {
                if (min.send(env, ">"_s, { max }).is_true()) {
                    env->raise("ArgumentError", "invalid argument - {}", arg->inspect_str(env));
                }

                if (min.is_float() || max.is_float()) {
                    double min_rand, max_rand;
                    if (min.is_float()) {
                        min_rand = min->as_float()->to_double();
                    } else {
                        min_rand = static_cast<double>(IntegerObject::convert_to_native_type<nat_int_t>(env, min));
                    }

                    if (max.is_float()) {
                        max_rand = max->as_float()->to_double();
                    } else {
                        max_rand = static_cast<double>(IntegerObject::convert_to_native_type<nat_int_t>(env, max));
                    }

                    return generate_random(min_rand, max_rand);
                } else {
                    nat_int_t min_rand = IntegerObject::convert_to_native_type<nat_int_t>(env, min);
                    nat_int_t max_rand = IntegerObject::convert_to_native_type<nat_int_t>(env, max);

                    if (arg->as_range()->exclude_end()) {
                        max_rand -= 1;
                    }

                    return generate_random(min_rand, max_rand);
                }
            }
            env->raise("ArgumentError", "bad value for range");
        }

        auto Numeric = GlobalEnv::the()->Object()->const_get("Numeric"_s);
        if (!arg.is_a(env, Numeric))
            env->raise("ArgumentError", "No implicit conversion of {} into Integer", arg.klass()->inspect_str());

        nat_int_t max = IntegerObject::convert_to_nat_int_t(env, arg);
        if (max <= 0)
            return generate_random(0.0, 1.0);
        return generate_random(0, max - 1);
    }
}

Value init_securerandom(Env *env, Value self) {
    return NilObject::the();
}
