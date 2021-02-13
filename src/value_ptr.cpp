#include "natalie.hpp"

namespace Natalie {

ValuePtr ValuePtr::send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    return value()->_send(env, name, argc, args, block);
}

ValuePtr ValuePtr::send(Env *env, const char *name, size_t argc, ValuePtr *args, Block *block) {
    return value()->_send(env, name, argc, args, block);
}

ValuePtr ValuePtr::send(Env *env, size_t argc, ValuePtr *args, Block *block) {
    return value()->_send(env, argc, args, block);
}

}
