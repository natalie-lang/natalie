#include "natalie.hpp"

namespace Natalie {

SymbolObject *SymbolObject::intern(const char *name, const size_t length, EncodingObject *encoding) {
    assert(name);
    return intern(String(name, length), encoding);
}

SymbolObject *SymbolObject::intern(const String &name, EncodingObject *encoding) {
    SymbolObject *symbol = s_symbols.get(name);
    if (symbol)
        return symbol;
    symbol = new SymbolObject { name, encoding };
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

StringObject *SymbolObject::to_s(Env *env) {
    if (m_encoding == nullptr) {
        return new StringObject { m_name };
    } else {
        return new StringObject { m_name, m_encoding };
    }
}

StringObject *SymbolObject::inspect(Env *env) {
    StringObject *string = new StringObject { ":" };

    if (should_be_quoted()) {
        StringObject *quoted = StringObject { m_name }.inspect(env);
        string->append(quoted);
    } else {
        string->append(m_name);
    }
    return string;
}

bool SymbolObject::should_be_quoted() const {
    if (!s_inspect_quote_regex) {
        auto pattern = String(
            "\\A\\$(\\d|\\?|\\!|~)\\z|" // :$0 :$? :$!
            "\\A(@{0,2}|\\$)[a-z_][a-z0-9_]*[\\?\\!=]?\\z|" // :foo :@bar :baz=
            "\\A(%|==|\\!|\\!=|\\+|\\-|/|\\*{1,2}|<<?|>>?|\\[\\]\\=?|&)\\z" // :% :== :**
        );
        UChar *pat = (UChar *)pattern.c_str();
        assert(onig_new(&s_inspect_quote_regex, pat, pat + pattern.size(), ONIG_OPTION_IGNORECASE, ONIG_ENCODING_UTF_8, ONIG_SYNTAX_DEFAULT, nullptr) == ONIG_NORMAL);
    }

    auto unsigned_str = (unsigned char *)m_name.c_str();
    auto char_end = unsigned_str + m_name.size();
    int result = onig_search(s_inspect_quote_regex, unsigned_str, char_end, unsigned_str, char_end, nullptr, ONIG_OPTION_NONE);

    return result < 0;
}

String SymbolObject::dbg_inspect() const {
    return String::format(":{}", m_name);
}

Value SymbolObject::eqtilde(Env *env, Value other) {
    other->assert_type(env, Object::Type::Regexp, "Regexp");
    return other->as_regexp()->eqtilde(env, this);
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

SymbolObject *SymbolObject::downcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "downcase"_s)->as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::swapcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "swapcase"_s)->as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::capitalize(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "capitalize"_s)->as_string();
    return string->to_symbol(env);
}
Value SymbolObject::casecmp(Env *env, Value other) {
    if (!other->is_symbol()) return NilObject::the();
    auto str1 = to_s(env);
    auto str2 = other->to_s(env);
    str1 = str1->send(env, "downcase"_s, { "ascii"_s })->as_string();
    str2 = str2->send(env, "downcase"_s, { "ascii"_s })->as_string();
    return str1->cmp(env, Value(str2));
}

Value SymbolObject::is_casecmp(Env *env, Value other) {
    if (!other->is_symbol()) return NilObject::the();
    // other->assert_type(env, Object::Type::Symbol, "Symbol");
    auto str1 = to_s(env);
    auto str2 = other->to_s(env);
    str1 = str1->send(env, "downcase"_s, { "ascii"_s })->as_string();
    str2 = str2->send(env, "downcase"_s, { "ascii"_s })->as_string();
    if (str1->string() == str2->string())
        return TrueObject::the();
    return FalseObject::the();
}

ProcObject *SymbolObject::to_proc(Env *env) {
    auto block_env = new Env {};
    block_env->var_set("name", 0, true, this);
    Block *proc_block = new Block { block_env, this, SymbolObject::to_proc_block_fn, -2, Block::BlockType::Lambda };
    return new ProcObject { proc_block };
}

Value SymbolObject::to_proc_block_fn(Env *env, Value self_value, Args args, Block *block) {
    args.ensure_argc_at_least(env, 1);
    SymbolObject *name_obj = env->outer()->var_get("name", 0)->as_symbol();
    assert(name_obj);
    auto method_args = Args::shift(args);
    return args[0].send(env, name_obj, method_args);
}

Value SymbolObject::cmp(Env *env, Value other_value) {
    if (!other_value->is_symbol()) return NilObject::the();
    SymbolObject *other = other_value->as_symbol();
    return Value::integer(m_name.cmp(other->m_name));
}

bool SymbolObject::start_with(Env *env, Args args) {
    return to_s(env)->start_with(env, args);
}

bool SymbolObject::end_with(Env *env, Args args) {
    return to_s(env)->end_with(env, args);
}

Value SymbolObject::length(Env *env) {
    return to_s(env)->size(env);
}

Value SymbolObject::match(Env *env, Value other, Block *block) const {
    other->assert_type(env, Object::Type::Regexp, "Regexp");
    return other->as_regexp()->match(env, name(env), nullptr, block);
}

bool SymbolObject::has_match(Env *env, Value other, Value start) const {
    other->assert_type(env, Object::Type::Regexp, "Regexp");
    return other->as_regexp()->has_match(env, name(env), start);
}

Value SymbolObject::name(Env *env) const {
    SymbolObject *symbol = intern(m_name);
    if (!symbol->m_string) {
        symbol->m_string = symbol->to_s(env);
        symbol->m_string->freeze();
    }
    return symbol->m_string;
}
Value SymbolObject::ref(Env *env, Value index_obj, Value length_obj) {
    // The next line worked in nearly every case, except it did not set `$~`
    // return to_s(env)->send(env, intern("[]"), { index_obj, length_obj });
    return to_s(env)->ref(env, index_obj, length_obj);
}

}
