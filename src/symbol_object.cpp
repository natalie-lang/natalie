#include "natalie.hpp"

namespace Natalie {

SymbolObject *SymbolObject::intern(const char *name, size_t length) {
    assert(name);
    return intern(String(name, length));
}

SymbolObject *SymbolObject::intern(const ManagedString *name) {
    assert(name);
    return intern(*name);
}

SymbolObject *SymbolObject::intern(const String &name) {
    SymbolObject *symbol = s_symbols.get(name);
    if (symbol)
        return symbol;
    symbol = new SymbolObject { name };
    s_symbols.put(name, symbol);
    return symbol;
}

ArrayObject *SymbolObject::all_symbols(Env *env) {
    ArrayObject *array = new ArrayObject(s_symbols.size());
    for (auto pair : s_symbols) {
        array->push(pair.second);
    }
    return array;
}

StringObject *SymbolObject::inspect(Env *env) {
    StringObject *string = new StringObject { ":" };
    // FIXME: surely we can do this without a regex
    auto quote_regex = RegexpObject { env, "\\A\\$(\\d|\\?|\\!|~)\\z|\\A(@{0,2}|\\$)[a-z_][a-z0-9_]*[\\?\\!=]?\\z|\\A(%|==|\\!|\\!=|\\+|\\-|/|\\*{1,2}|<<?|>>?|\\[\\]\\=?|&)\\z", 1 };
    bool quote = quote_regex.match(env, new StringObject { m_name })->is_falsey();
    for (size_t i = 0; i < m_name.length(); ++i) {
        auto c = m_name[i];
        if (c < 33 || c > 126) // FIXME: probably can be a smaller range
            quote = true;
    }
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

Value SymbolObject::to_proc_block_fn(Env *env, Value self_value, Args args, Block *block) {
    env->ensure_argc_is(args.argc, 1);
    SymbolObject *name_obj = env->outer()->var_get("name", 0)->as_symbol();
    assert(name_obj);
    return args[0].send(env, name_obj);
}

Value SymbolObject::cmp(Env *env, Value other_value) {
    if (!other_value->is_symbol()) return NilObject::the();
    SymbolObject *other = other_value->as_symbol();
    return Value::integer(m_name.cmp(other->m_name));
}

bool SymbolObject::start_with(Env *env, Value needle) {
    return to_s(env)->internal_start_with(env, needle);
}

Value SymbolObject::length(Env *env) {
    return to_s(env)->size(env);
}

Value SymbolObject::name(Env *env) {
    SymbolObject *symbol = intern(m_name);
    if (!symbol->m_string) {
        symbol->m_string = symbol->to_s(env);
        symbol->m_string->freeze();
    }
    return symbol->m_string;
}

Value SymbolObject::ref(Env *env, Value index_obj) {
    return to_s(env)->send(env, intern("[]"), { index_obj });
}

}
