#include "natalie.hpp"

namespace Natalie {

SymbolValue *SymbolValue::intern(Env *env, const char *name) {
    SymbolValue *symbol = static_cast<SymbolValue *>(hashmap_get(env->global_env->symbols, name));
    if (symbol) {
        return symbol;
    } else {
        symbol = new SymbolValue { env, name };
        hashmap_put(env->global_env->symbols, name, symbol);
        return symbol;
    }
}

StringValue *SymbolValue::inspect(Env *env) {
    StringValue *string = new StringValue { env, ":" };
    ssize_t len = strlen(m_name);
    bool quote = false;
    for (ssize_t i = 0; i < len; i++) {
        char c = m_name[i];
        if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == '_')) {
            quote = true;
            break;
        }
    };
    if (quote) {
        StringValue *quoted = StringValue { env, m_name }.inspect(env);
        string->append_string(env, quoted);
    } else {
        string->append(env, m_name);
    }
    return string;
}

}
