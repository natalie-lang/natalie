#include "natalie.hpp"
#include <grp.h>
#include <unistd.h>

using namespace Natalie;

Value passwd_to_struct(Env *env, Value self, struct passwd *pwd) {
    auto etc_passwd = self->const_get("Passwd"_s);
    assert(etc_passwd);
    auto pwdstruct = etc_passwd->send(env, "new"_s, {});
    // It is possible more fields could be set, but these
    // are the ones required by POSIX
    pwdstruct->send(env, "name="_s, { new StringObject { pwd->pw_name } });
    pwdstruct->send(env, "uid="_s, { new IntegerObject { pwd->pw_uid } });
    pwdstruct->send(env, "gid="_s, { new IntegerObject { pwd->pw_gid } });
    pwdstruct->send(env, "dir="_s, { new StringObject { pwd->pw_dir } });
    pwdstruct->send(env, "shell="_s, { new StringObject { pwd->pw_dir } });
    return pwdstruct;
}

Value init(Env *env, Value self) {
    ModuleObject *Etc = new ModuleObject { "Etc" };
    GlobalEnv::the()->Object()->const_set("Etc"_s, Etc);
    Etc->const_set("VERSION"_s, new StringObject { "1.3.0" });
    return NilObject::the();
}

Value Etc_getlogin(Env *env, Value self, Args args, Block *_block) {
    args.ensure_argc_is(env, 0);
    char *login = ::getlogin();
    if (!login) login = ::getenv("USER");
    if (!login) return NilObject::the();
    return new StringObject { login, EncodingObject::locale() };
}

Value Etc_getpwent(Env *env, Value self, Args args, Block *_block) {
    args.ensure_argc_is(env, 0);
    struct passwd *pwd = ::getpwent();
    if (pwd)
        return passwd_to_struct(env, self, pwd);
    return NilObject::the();
}

Value Etc_getpwnam(Env *env, Value self, Args args, Block *_block) {
    args.ensure_argc_is(env, 1);
    auto firstarg = args.at(0);
    firstarg->assert_type(env, Object::Type::String, "String");
    StringObject *username = firstarg->as_string();
    struct passwd *pwd = ::getpwnam(username->c_str());
    if (!pwd)
        env->raise("ArgumentError", "can't find user for {}", username->string());
    return passwd_to_struct(env, self, pwd);
}

Value Etc_getpwuid(Env *env, Value self, Args args, Block *_block) {
    args.ensure_argc_between(env, 0, 1);
    uid_t uid;
    if (args.size() == 1) {
        uid = IntegerObject::convert_to_nat_int_t(env, args.at(0));
    } else {
        uid = ::getuid();
    }
    struct passwd *pwd = ::getpwuid(uid);
    if (!pwd) env->raise_errno();
    return passwd_to_struct(env, self, pwd);
}

Value Etc_setgrent(Env *env, Value self, Args _args, Block *_block) {
    ::setgrent();
    return NilObject::the();
}

Value Etc_setpwent(Env *env, Value self, Args _args, Block *_block) {
    ::setpwent();
    return NilObject::the();
}
