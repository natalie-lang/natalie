#include "natalie.hpp"
#include "natalie/macros.hpp"

#include <errno.h>
#include <filesystem>
#include <sys/stat.h>

namespace Natalie {

void change_current_path(Env *env, std::filesystem::path path) {
    std::error_code ec;
    std::filesystem::current_path(path, ec);
    errno = ec.value();
    if (errno)
        env->raise_errno();
}

Value DirObject::chdir(Env *env, Value path, Block *block) {
    if (!path) {
        auto path_str = ::getenv("HOME");
        if (!path_str)
            path_str = ::getenv("LOGDIR");
        if (!path_str)
            env->raise("ArgumentError", "HOME/LOGDIR not set");

        path = new StringObject { path_str };
    }

    auto old_path = std::filesystem::current_path();
    auto new_path = std::filesystem::path { path->to_str(env)->c_str() };
    change_current_path(env, new_path);

    if (!block)
        return Value::integer(0);
    
    Value args[] = { path };
    Value result;
    try {
        result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    } catch (ExceptionObject *exception) {
        change_current_path(env, old_path);
        throw exception;
    }

    change_current_path(env, old_path);
    return result;
}

Value DirObject::mkdir(Env *env, Value path, Value mode) {
    int octmode = 0777;
    path->assert_type(env, Object::Type::String, "String");
    auto result = ::mkdir(path->as_string()->c_str(), octmode);
    if (result < 0) env->raise_errno();
    // need to check dir exists and return nil if mkdir was unsuccessful.
    return Value::integer(0);
}

Value DirObject::pwd(Env *env) {
    return new StringObject { std::filesystem::current_path().c_str() };
}

Value DirObject::rmdir(Env *env, Value path) {
    path->assert_type(env, Object::Type::String, "String");
    auto result = ::rmdir(path->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

}
