#include "natalie.hpp"
#include <dlfcn.h>

using namespace Natalie;

Value init_shared_library(Env *env, Value self) {
    return Value::nil();
}

Value LlvmSharedLibrary_load(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 3);
    // Ruby methods and literal roots can retain addresses in the library. Keep
    // the handle loaded for the remainder of this test process.
    auto handle = dlopen(args[0].as_string()->c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle)
        env->raise("RuntimeError", "dlopen: {}", dlerror());
    auto initialize = reinterpret_cast<Value (*)(Env *, Value)>(dlsym(handle, args[1].as_string()->c_str()));
    if (!initialize)
        env->raise("RuntimeError", "dlsym: {}", dlerror());
    return initialize(Env::create(env), args[2]);
}
