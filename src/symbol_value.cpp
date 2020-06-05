#include "natalie.hpp"

namespace Natalie {

SymbolValue *StringValue::to_symbol(Env *env) {
    return symbol(env, str);
}

}
