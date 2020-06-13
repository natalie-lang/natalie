#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Regexp_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (args[0]->is_regexp()) {
        return new RegexpValue { env, args[0]->as_regexp()->pattern() };
    } else {
        NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
        return new RegexpValue { env, args[0]->as_string()->c_str() };
    }
}

Value *Regexp_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (*self == *arg) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Regexp_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(0);
    StringValue *out = new StringValue { env, "/" };
    out->append(env, self->pattern());
    out->append_char(env, '/');
    return out;
}

Value *Regexp_eqtilde(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
    Value *match = Regexp_match(env, self, argc, args, block);
    if (match->is_nil()) {
        return match;
    } else {
        MatchDataValue *matchdata = match->as_match_data();
        assert(matchdata->matchdata_region->num_regs > 0);
        return new IntegerValue { env, matchdata->matchdata_region->beg[0] };
    }
}

Value *Regexp_match(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
    StringValue *str_obj = args[0]->as_string();

    OnigRegion *region = onig_region_new();
    int result = self->search(str_obj->c_str(), region, ONIG_OPTION_NONE);
    if (result >= 0) {
        env->caller->match = matchdata_new(env, region, str_obj);
        return env->caller->match;
    } else if (result == ONIG_MISMATCH) {
        env->caller->match = NULL;
        onig_region_free(region, true);
        return NAT_NIL;
    } else {
        env->caller->match = NULL;
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, "RuntimeError", (char *)s);
    }
}

}
