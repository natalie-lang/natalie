#include <unistd.h>
#include <grp.h>
#include "natalie.hpp"

using namespace Natalie;

Value init(Env *env, Value self) {
    ModuleObject *Etc = new ModuleObject { "Etc" };
    GlobalEnv::the()->Object()->const_set("Etc"_s, Etc);
    Etc->const_set("VERSION"_s, new StringObject { "1.3.0" });
    return NilObject::the();
}

Value Etc_getlogin(Env *env, Value self, Args _args, Block* _block) {
    char* login = ::getlogin();
    if (!login) login = ::getenv("USER");
    if (!login) return NilObject::the();
    return new StringObject { login, EncodingObject::locale() };
}

Value Etc_setgrent(Env *env, Value self, Args _args, Block* _block) {
    ::setgrent();
    return NilObject::the();
}

Value Etc_setpwent(Env *env, Value self, Args _args, Block* _block) {
    ::setpwent();
    return NilObject::the();
}

