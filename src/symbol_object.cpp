#include "natalie.hpp"

namespace Natalie {

SymbolObject *SymbolObject::intern(const char *name, Ownedness ownedness) {
    assert(name);
    SymbolObject *symbol = s_symbols.get(name);
    if (symbol)
        return symbol;
    symbol = new SymbolObject { name, ownedness };
    s_symbols.put(name, symbol);
    return symbol;
}

SymbolObject *SymbolObject::intern(const String *name) {
    assert(name);
    return intern(name->c_str(), Ownedness::DuplicatedString);
}

ArrayObject *SymbolObject::all_symbols(Env *env) {
    ArrayObject *array = new ArrayObject(s_symbols.size());
    for (auto pair : s_symbols) {
        Value symbol_value = pair.second;
        array->push(env, 1, &symbol_value);
    }
    return array;
}

StringObject *SymbolObject::inspect(Env *env) {
    StringObject *string = new StringObject { ":" };
    auto quote_regex = new RegexpObject { env, "\\A\\$(\\d|\\?|\\!|~)\\z|\\A(@{0,2}|\\$)[a-z_][a-z0-9_]*[\\?\\!=]?\\z|\\A(%|==|\\!|\\!=|\\+|\\-|/|\\*{1,2}|<<?|>>?|\\[\\]\\=?|&)\\z", 1 };
    bool quote = quote_regex->match(env, new StringObject { m_name })->is_falsey();
    if (quote) {
        StringObject *quoted = StringObject { m_name }.inspect(env);
        string->append(env, quoted);
    } else {
        string->append(env, m_name);
    }
    return string;
}

SymbolObject *SymbolObject::succ(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "succ"_s)->as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::upcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "upcase"_s)->as_string();
    return string->to_symbol(env);
}

ProcObject *SymbolObject::to_proc(Env *env) {
    auto block_env = new Env {};
    block_env->var_set("name", 0, true, this);
    Block *proc_block = new Block { block_env, this, SymbolObject::to_proc_block_fn, 1 };
    return new ProcObject { proc_block };
}

Value SymbolObject::to_proc_block_fn(Env *env, Value self_value, size_t argc, Value *args, Block *block) {
    env->ensure_argc_is(argc, 1);
    SymbolObject *name_obj = env->outer()->var_get("name", 0)->as_symbol();
    assert(name_obj);
    return args[0].send(env, name_obj);
}

Value SymbolObject::cmp(Env *env, Value other_value) {
    if (!other_value->is_symbol()) return NilObject::the();
    SymbolObject *other = other_value->as_symbol();
    if (this == other)
        return Value::integer(0);
    int diff = strcmp(m_name, other->m_name);
    int result;
    if (diff < 0) {
        result = -1;
    } else if (diff > 0) {
        result = 1;
    } else {
        NAT_UNREACHABLE();
    }
    return Value::integer(result);
}

bool SymbolObject::start_with(Env *env, Value needle) {
    return to_s(env)->start_with(env, needle);
}

Value SymbolObject::ref(Env *env, Value index_obj) {
    return to_s(env)->send(env, intern("[]"), { index_obj });
}

}
