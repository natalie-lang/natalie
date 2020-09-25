#include "natalie.hpp"

namespace Natalie {

Value *RegexpValue::initialize(Env *env, Value *arg) {
    if (arg->is_regexp()) {
        m_regex = arg->as_regexp()->m_regex;
        m_pattern = arg->as_regexp()->m_pattern;
    } else {
        NAT_ASSERT_TYPE(arg, Value::Type::String, "String");
        initialize(env, arg->as_string()->c_str());
    }
    return this;
}

Value *RegexpValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "/" };
    out->append(env, pattern());
    out->append_char(env, '/');
    if ((options() & 4) != 0) out->append_char(env, 'm');
    if ((options() & 1) != 0) out->append_char(env, 'i');
    if ((options() & 2) != 0) out->append_char(env, 'x');
    return out;
}

Value *RegexpValue::eqtilde(Env *env, Value *other) {
    NAT_ASSERT_TYPE(other, Value::Type::String, "String");
    Value *result = match(env, other);
    if (result->is_nil()) {
        return result;
    } else {
        MatchDataValue *matchdata = result->as_match_data();
        assert(matchdata->size() > 0);
        return new IntegerValue { env, matchdata->index(0) };
    }
}

Value *RegexpValue::match(Env *env, Value *other) {
    NAT_ASSERT_TYPE(other, Value::Type::String, "String");
    StringValue *str_obj = other->as_string();

    OnigRegion *region = onig_region_new();
    int result = search(str_obj->c_str(), region, ONIG_OPTION_NONE);
    Env *caller_env = env->caller();
    if (result >= 0) {
        caller_env->set_match(new MatchDataValue { env, region, str_obj });
        return caller_env->match();
    } else if (result == ONIG_MISMATCH) {
        caller_env->clear_match();
        onig_region_free(region, true);
        return env->nil_obj();
    } else {
        caller_env->clear_match();
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, "RuntimeError", (char *)s);
    }
}

}
