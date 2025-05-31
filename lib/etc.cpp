#include "natalie.hpp"
#include <grp.h>
#include <sys/utsname.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

using namespace Natalie;

Value group_to_struct(Env *env, Value self, struct group *grp) {
    auto etc_group = Object::const_fetch(self, "Group"_s);
    auto grpstruct = etc_group.send(env, "new"_s, {});
    // It is possible more fields could be set, but these
    // are the ones required by POSIX
    grpstruct.send(env, "name="_s, { StringObject::create(grp->gr_name) });
    grpstruct.send(env, "passwd="_s, { StringObject::create(grp->gr_passwd) });
    grpstruct.send(env, "gid="_s, { Value::integer(grp->gr_gid) });
    auto mem_ary = ArrayObject::create();
    char **memptr = grp->gr_mem;
    for (char *member_name = *memptr; member_name; member_name = *++memptr) {
        auto memstr = StringObject::create(member_name);
        mem_ary->push(Value(memstr));
    }
    grpstruct.send(env, "mem="_s, { mem_ary });
    return grpstruct;
}

Value passwd_to_struct(Env *env, Value self, struct passwd *pwd) {
    auto etc_passwd = Object::const_fetch(self, "Passwd"_s);
    auto pwdstruct = etc_passwd.send(env, "new"_s, {});
    // It is possible more fields could be set, but these
    // are the ones required by POSIX
    pwdstruct.send(env, "name="_s, { StringObject::create(pwd->pw_name) });
    pwdstruct.send(env, "uid="_s, { Value::integer(pwd->pw_uid) });
    pwdstruct.send(env, "gid="_s, { Value::integer(pwd->pw_gid) });
    pwdstruct.send(env, "dir="_s, { StringObject::create(pwd->pw_dir) });
    pwdstruct.send(env, "gecos="_s, { StringObject::create(pwd->pw_gecos) });
    pwdstruct.send(env, "passwd="_s, { StringObject::create(pwd->pw_passwd) });
    pwdstruct.send(env, "shell="_s, { StringObject::create(pwd->pw_shell) });
    return pwdstruct;
}

Value init_etc(Env *env, Value self) {
    return Value::nil();
}

Value Etc_confstr(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    const int name = IntegerMethods::convert_to_nat_int_t(env, args[0]);
    const auto size = ::confstr(name, nullptr, 0);
    if (size == 0) {
        if (errno)
            env->raise_errno();
        return Value::nil();
    }
    TM::String buf(size - 1, '\0');
    if (!::confstr(name, &buf[0], size))
        env->raise_errno();
    return StringObject::create(std::move(buf));
}

Value Etc_endgrent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    ::endgrent();
    return Value::nil();
}
Value Etc_endpwent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    ::endpwent();
    return Value::nil();
}

Value Etc_getgrent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    struct group *grp = ::getgrent();
    if (!grp) return Value::nil();
    return group_to_struct(env, self, grp);
}

Value Etc_getgrgid(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_between(env, 0, 1);
    uid_t uid;
    if (args.size() == 1) {
        uid = IntegerMethods::convert_to_nat_int_t(env, args.at(0));
    } else {
        uid = ::getgid();
    }
    struct group *grp = ::getgrgid(uid);
    if (!grp) env->raise_errno();
    return group_to_struct(env, self, grp);
}

Value Etc_getgrnam(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 1);
    auto firstarg = args.at(0);
    firstarg.assert_type(env, Object::Type::String, "String");
    StringObject *groupname = firstarg.as_string();
    struct group *grp = ::getgrnam(groupname->c_str());
    if (!grp)
        env->raise("ArgumentError", "can't find group for {}", groupname->string());
    return group_to_struct(env, self, grp);
}

//
Value Etc_getlogin(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    char *login = ::getlogin();
    if (!login) login = ::getenv("USER");
    if (!login) return Value::nil();
    return StringObject::create(login, EncodingObject::locale());
}

Value Etc_getpwent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    struct passwd *pwd = ::getpwent();
    if (!pwd) return Value::nil();
    return passwd_to_struct(env, self, pwd);
}

Value Etc_getpwnam(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 1);
    auto firstarg = args.at(0);
    firstarg.assert_type(env, Object::Type::String, "String");
    StringObject *user_name = firstarg.as_string();
    struct passwd *pwd = ::getpwnam(user_name->c_str());
    if (!pwd)
        env->raise("ArgumentError", "can't find user for {}", user_name->string());
    return passwd_to_struct(env, self, pwd);
}

Value Etc_getpwuid(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_between(env, 0, 1);
    uid_t uid;
    if (args.size() == 1) {
        uid = IntegerMethods::convert_to_nat_int_t(env, args.at(0));
    } else {
        uid = ::getuid();
    }
    struct passwd *pwd = ::getpwuid(uid);
    if (!pwd) env->raise_errno();
    return passwd_to_struct(env, self, pwd);
}

Value Etc_nprocessors(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    const auto result = sysconf(_SC_NPROCESSORS_ONLN);
    if (result < 0) env->raise_errno();
    return Value::integer(result);
}

Value Etc_setgrent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    ::setgrent();
    return Value::nil();
}

Value Etc_setpwent(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 0);
    ::setpwent();
    return Value::nil();
}

Value Etc_sysconf(Env *env, Value self, Args &&args, Block *_block) {
    args.ensure_argc_is(env, 1);
    nat_int_t nameval = IntegerMethods::convert_to_nat_int_t(env, args.at(0));
    errno = 0;
    long status = ::sysconf(nameval);
    if (status < 0) {
        if (errno) env->raise_errno();
        return Value::nil();
    }
    return Value::integer(status);
}

Value Etc_systmpdir(Env *env, Value, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

#ifdef __APPLE__
    // https://forums.developer.apple.com/forums/thread/71382
    char buf[PATH_MAX + 1];
    const auto len = confstr(_CS_DARWIN_USER_TEMP_DIR, buf, PATH_MAX);
    return StringObject::create(buf, len, Encoding::ASCII_8BIT);
#else
    return StringObject::create("/tmp", Encoding::ASCII_8BIT);
#endif
}

Value Etc_uname(Env *env, Value, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    utsname buf;
    if (uname(&buf) < 0)
        env->raise_errno();

    auto result = HashObject::create();
    result->put(env, SymbolObject::intern("sysname"), StringObject::create(buf.sysname));
    result->put(env, SymbolObject::intern("nodename"), StringObject::create(buf.nodename));
    result->put(env, SymbolObject::intern("release"), StringObject::create(buf.release));
    result->put(env, SymbolObject::intern("version"), StringObject::create(buf.version));
    result->put(env, SymbolObject::intern("machine"), StringObject::create(buf.machine));
    return result;
}
