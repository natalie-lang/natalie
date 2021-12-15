#include "natalie.hpp"

namespace Natalie {

ValuePtr RegexpObject::initialize(Env *env, ValuePtr arg) {
    if (arg->is_regexp()) {
        auto other = arg->as_regexp();
        initialize(env, other->pattern(), other->options());
    } else {
        arg->assert_type(env, Object::Type::String, "String");
        initialize(env, arg->as_string()->c_str());
    }
    return this;
}

ValuePtr RegexpObject::inspect(Env *env) {
    StringObject *out = new StringObject { "/" };
    const char *str = pattern();
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        switch (c) {
        case '\n':
            out->append(env, "\\n");
            break;
        case '\t':
            out->append(env, "\\t");
            break;
        default:
            out->append_char(c);
        }
    }
    out->append_char('/');
    if ((options() & 4) != 0) out->append_char('m');
    if ((options() & 1) != 0) out->append_char('i');
    if ((options() & 2) != 0) out->append_char('x');
    return out;
}

ValuePtr RegexpObject::eqtilde(Env *env, ValuePtr other) {
    if (other->is_symbol())
        other = other->as_symbol()->to_s(env);
    other->assert_type(env, Object::Type::String, "String");
    ValuePtr result = match(env, other);
    if (result->is_nil()) {
        return result;
    } else {
        MatchDataObject *matchdata = result->as_match_data();
        assert(matchdata->size() > 0);
        return IntegerObject::from_size_t(env, matchdata->index(0));
    }
}

ValuePtr RegexpObject::match(Env *env, ValuePtr other, size_t start_index) {
    if (other->is_symbol())
        other = other->as_symbol()->to_s(env);
    other->assert_type(env, Object::Type::String, "String");
    StringObject *str_obj = other->as_string();

    OnigRegion *region = onig_region_new();
    int result = search(str_obj->c_str(), start_index, region, ONIG_OPTION_NONE);

    Env *caller_env = env->caller();

    if (result >= 0) {
        auto match = new MatchDataObject { region, str_obj };
        caller_env->set_last_match(match);
        return match;
    } else if (result == ONIG_MISMATCH) {
        caller_env->clear_match();
        onig_region_free(region, true);
        return NilObject::the();
    } else {
        caller_env->clear_match();
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        env->raise("RuntimeError", (char *)s);
    }
}

ValuePtr RegexpObject::source(Env *env) {
    return new StringObject { pattern() };
}

}
