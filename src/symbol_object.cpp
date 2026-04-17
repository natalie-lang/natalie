#include "natalie.hpp"
#include "tm/owned_ptr.hpp"

namespace Natalie {

SymbolObject *SymbolObject::intern(const char *name, const size_t length, EncodingObject *encoding) {
    assert(name);
    return intern(String(name, length), encoding);
}

SymbolObject *SymbolObject::intern(const String &name, EncodingObject *encoding) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    SymbolObject *symbol = s_symbols.get(name);
    if (symbol)
        return symbol;
    symbol = new SymbolObject { name, encoding };
    s_symbols.put(name, symbol);
    return symbol;
}

ArrayObject *SymbolObject::all_symbols(Env *env) {
    ArrayObject *array = ArrayObject::create(s_symbols.size());
    for (auto pair : s_symbols) {
        array->push(pair.second);
    }
    return array;
}

StringObject *SymbolObject::to_s(Env *env) {
    StringObject *result = nullptr;
    if (m_encoding == nullptr) {
        result = StringObject::create(m_name);
    } else {
        result = StringObject::create(m_name, m_encoding);
    }
    result->set_chilled(StringObject::Chilled::Symbol);
    return result;
}

StringObject *SymbolObject::inspect(Env *env) {
    StringObject *string = StringObject::create(":");

    if (is_invalid_name()) {
        auto quoted = StringObject::create(m_name)->inspect(env);
        string->append(quoted);
    } else {
        string->append(m_name);
    }
    return string;
}

SymbolObject::NameType SymbolObject::classify_name() const {
    const auto size = m_name.size();
    if (size == 0) return NameType::Invalid;
    const auto *s = (const unsigned char *)m_name.c_str();

    const bool non_ascii_ok = m_encoding
        && m_encoding->num() != Encoding::ASCII_8BIT
        && m_encoding->is_ascii_compatible()
        && !m_encoding->is_dummy();

    auto ident_start = [&](unsigned char c) {
        return c >= 0x80 ? non_ascii_ok : (is_ascii_alpha(c) || c == '_');
    };
    auto ident_cont = [&](unsigned char c) {
        return c >= 0x80 ? non_ascii_ok : (is_ascii_alnum(c) || c == '_');
    };

    const auto c0 = s[0];

    switch (c0) {
    case '$': {
        if (size == 1) return NameType::Invalid;
        const auto c1 = s[1];
        if (size == 2 && strchr("!?~&`'+=\\/,;.<>:\"*$@_", c1)) return NameType::Global;
        if (c1 == '-') {
            if (size != 3 || !ident_cont(s[2])) return NameType::Invalid;
            return NameType::Global;
        }
        if (is_ascii_digit(c1)) {
            for (size_t i = 2; i < size; i++)
                if (!is_ascii_digit(s[i])) return NameType::Invalid;
            return NameType::Global;
        }
        if (!ident_start(c1)) return NameType::Invalid;
        for (size_t i = 2; i < size; i++)
            if (!ident_cont(s[i])) return NameType::Invalid;
        return NameType::Global;
    }
    case '@': {
        const bool cvar = size > 1 && s[1] == '@';
        const size_t start = cvar ? 2 : 1;
        if (start >= size || !ident_start(s[start])) return NameType::Invalid;
        for (size_t i = start + 1; i < size; i++)
            if (!ident_cont(s[i])) return NameType::Invalid;
        return cvar ? NameType::CVar : NameType::IVar;
    }
    case '!':
        if (size == 1) return NameType::Junk;
        if (size == 2 && (s[1] == '=' || s[1] == '~')) return NameType::Junk;
        return NameType::Invalid;
    case '%':
    case '&':
    case '/':
    case '^':
    case '`':
    case '|':
    case '~':
        return size == 1 ? NameType::Junk : NameType::Invalid;
    case '*':
        if (size == 1) return NameType::Junk;
        if (size == 2 && s[1] == '*') return NameType::Junk;
        return NameType::Invalid;
    case '+':
    case '-':
        if (size == 1) return NameType::Junk;
        if (size == 2 && s[1] == '@') return NameType::Junk;
        return NameType::Invalid;
    case '<':
        if (size == 1) return NameType::Junk;
        if (size == 2 && (s[1] == '<' || s[1] == '=')) return NameType::Junk;
        if (size == 3 && s[1] == '=' && s[2] == '>') return NameType::Junk;
        return NameType::Invalid;
    case '>':
        if (size == 1) return NameType::Junk;
        if (size == 2 && (s[1] == '>' || s[1] == '=')) return NameType::Junk;
        return NameType::Invalid;
    case '=':
        if (size == 2 && (s[1] == '=' || s[1] == '~')) return NameType::Junk;
        if (size == 3 && s[1] == '=' && s[2] == '=') return NameType::Junk;
        return NameType::Invalid;
    case '[':
        if (size == 2 && s[1] == ']') return NameType::Junk;
        if (size == 3 && s[1] == ']' && s[2] == '=') return NameType::Junk;
        return NameType::Invalid;
    default: {
        if (!ident_start(c0)) return NameType::Invalid;
        size_t i = 1;
        while (i < size && ident_cont(s[i]))
            i++;
        if (i == size)
            return is_ascii_upper(c0) ? NameType::Const : NameType::Local;
        if (i + 1 == size) {
            switch (s[i]) {
            case '=':
                return NameType::AttrSet;
            case '?':
            case '!':
                return NameType::Junk;
            }
        }
        return NameType::Invalid;
    }
    }
}

String SymbolObject::dbg_inspect(int indent) const {
    return String::format("<SymbolObject {h} name=\"{}\">", this, m_name);
}

Value SymbolObject::eqtilde(Env *env, Value other) {
    other.assert_type(env, Object::Type::Regexp, "Regexp");
    return other.as_regexp()->eqtilde(env, this);
}

SymbolObject *SymbolObject::succ(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "succ"_s).as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::upcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "upcase"_s).as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::downcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "downcase"_s).as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::swapcase(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "swapcase"_s).as_string();
    return string->to_symbol(env);
}

SymbolObject *SymbolObject::capitalize(Env *env) {
    auto string = to_s(env);
    string = string->send(env, "capitalize"_s).as_string();
    return string->to_symbol(env);
}
Value SymbolObject::casecmp(Env *env, Value other) {
    if (!other.is_symbol()) return Value::nil();
    auto str1 = to_s(env);
    auto str2 = other.to_s(env);
    str1 = str1->send(env, "downcase"_s, { "ascii"_s }).as_string();
    str2 = str2->send(env, "downcase"_s, { "ascii"_s }).as_string();
    return str1->cmp(env, Value(str2));
}

Value SymbolObject::is_casecmp(Env *env, Value other) {
    if (!other.is_symbol()) return Value::nil();
    // other.assert_type(env, Object::Type::Symbol, "Symbol");
    auto str1 = to_s(env);
    auto str2 = other.to_s(env);
    str1 = str1->send(env, "downcase"_s, { "ascii"_s }).as_string();
    str2 = str2->send(env, "downcase"_s, { "ascii"_s }).as_string();
    if (str1->string() == str2->string())
        return Value::True();
    return Value::False();
}

ProcObject *SymbolObject::to_proc(Env *env) {
    OwnedPtr<Env> block_env { Env::create() };
    block_env->var_set("name", 0, true, this);
    Block *proc_block = Block::create(std::move(block_env), this, SymbolObject::to_proc_block_fn, -2, false, Block::BlockType::Lambda);
    return ProcObject::create(proc_block);
}

Value SymbolObject::to_proc_block_fn(Env *env, Value self_value, Args &&args, Block *block) {
    args.ensure_argc_at_least(env, 1);
    SymbolObject *name_obj = env->outer()->var_get("name", 0).as_symbol();
    assert(name_obj);
    auto receiver = args.shift(env, true);
    return receiver.public_send(env, name_obj, std::move(args));
}

Value SymbolObject::cmp(Env *env, Value other_value) {
    if (!other_value.is_symbol()) return Value::nil();
    SymbolObject *other = other_value.as_symbol();
    return Value::integer(m_name.cmp(other->m_name));
}

bool SymbolObject::start_with(Env *env, Args &&args) {
    return to_s(env)->start_with(env, std::move(args));
}

bool SymbolObject::end_with(Env *env, Args &&args) {
    return to_s(env)->end_with(env, std::move(args));
}

Value SymbolObject::length(Env *env) {
    return to_s(env)->size(env);
}

Value SymbolObject::match(Env *env, Value other, Block *block) const {
    other.assert_type(env, Object::Type::Regexp, "Regexp");
    return other.as_regexp()->match(env, name(env), {}, block);
}

bool SymbolObject::has_match(Env *env, Value other, Optional<Value> start) const {
    other.assert_type(env, Object::Type::Regexp, "Regexp");
    return other.as_regexp()->has_match(env, name(env), start.value_or(Value::nil()));
}

Value SymbolObject::name(Env *env) const {
    SymbolObject *symbol = intern(m_name);
    if (!symbol->m_string) {
        symbol->m_string = symbol->to_s(env);
        symbol->m_string->freeze();
    }
    return symbol->m_string;
}
Value SymbolObject::ref(Env *env, Value index_obj, Optional<Value> length_obj) {
    // The next line worked in nearly every case, except it did not set `$~`
    // return to_s(env).send(env, intern("[]"), { index_obj, length_obj });
    return to_s(env)->ref(env, index_obj, length_obj);
}

}
