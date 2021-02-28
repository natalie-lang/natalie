#include "natalie.hpp"

namespace Natalie {

SymbolValue *SymbolValue::intern(Env *env, const char *name) {
    assert(name);
    SymbolValue *symbol = env->global_env()->symbol_get(env, name);
    if (symbol) {
        return symbol;
    } else {
        symbol = new SymbolValue { env, name };
        env->global_env()->symbol_set(env, name, symbol);
        return symbol;
    }
}

StringValue *SymbolValue::inspect(Env *env) {
    StringValue *string = new StringValue { env, ":" };
    size_t len = strlen(m_name);
    auto quote_regex = new RegexpValue { env, "\\A\\$(\\d|\\?|\\!|~)\\z|\\A(@{0,2}|\\$)[a-z_][a-z0-9_]*[\\?\\!=]?\\z|\\A(%|==|\\!|\\!=|\\+|\\-|/|\\*{1,2}|<<?|>>?|\\[\\]\\=?|&)\\z", 1 };
    bool quote = quote_regex->match(env, new StringValue { env, m_name })->is_falsey();
    if (quote) {
        StringValue *quoted = StringValue { env, m_name }.inspect(env);
        string->append_string(env, quoted);
    } else {
        string->append(env, m_name);
    }
    return string;
}

ProcValue *SymbolValue::to_proc(Env *env) {
    Env block_env = Env::new_detatched_env(env);
    block_env.var_set("name", 0, true, this);
    Block *proc_block = new Block { block_env, this, SymbolValue::to_proc_block_fn, 1 };
    return new ProcValue { env, proc_block };
}

ValuePtr SymbolValue::to_proc_block_fn(Env *env, ValuePtr self_value, size_t argc, ValuePtr *args, Block *block) {
    env->assert_argc(argc, 1);
    SymbolValue *name_obj = env->outer()->var_get("name", 0)->as_symbol();
    assert(name_obj);
    const char *name = name_obj->c_str();
    return args[0].send(env, name);
}

ValuePtr SymbolValue::cmp(Env *env, ValuePtr other_value) {
    if (!other_value->is_symbol()) return env->nil_obj();
    SymbolValue *other = other_value->as_symbol();
    int diff = strcmp(m_name, other->m_name);
    int result;
    if (diff < 0) {
        result = -1;
    } else if (diff > 0) {
        result = 1;
    } else {
        result = 0;
    }
    return new IntegerValue { env, result };
}

bool SymbolValue::start_with(Env *env, ValuePtr needle) {
    return to_s(env)->start_with(env, needle);
}

ValuePtr SymbolValue::ref(Env *env, ValuePtr index_obj) {
    ValuePtr args[] = { index_obj };
    return to_s(env)->_send(env, "[]", 1, args);
}

}
