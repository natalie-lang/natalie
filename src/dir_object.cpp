#include "natalie.hpp"
#include "natalie/macros.hpp"

#include <errno.h>
#include <filesystem>
#include <sys/stat.h>

namespace Natalie {

// TODO: handle encoding
Value DirObject::open(Env *env, Value path, Value encoding, Block *block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    if (block) {
        // FIXME: break/exception not calling close()
        Value result;
        try {
            result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ dir }), nullptr);
        } catch (ExceptionObject *exception) {
            dir->close(env);
            throw;
        }
        dir->close(env);
        return result;
    }
    return dir;
}

// TODO: handle encoding
Value DirObject::initialize(Env *env, Value path, Value encoding) {
    path = fileutil::convert_using_to_path(env, path);
    m_dir = ::opendir(path->as_string()->c_str());
    if (!m_dir) env->raise_errno();
    m_path = path->as_string()->dup(env)->as_string();
    assert(m_path);
    return this;
}

// instance methods
Value DirObject::fileno(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    int fdnum = ::dirfd(m_dir);
    return new IntegerObject { fdnum };
}

Value DirObject::close(Env *env) {
    m_dir = nullptr;
    return NilObject::the();
}

// TODO: support encoding
Value DirObject::read(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    dirp = ::readdir(m_dir);
    if (!dirp) return NilObject::the();
    return new StringObject { dirp->d_name };
}

Value DirObject::tell(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    long position = ::telldir(m_dir);
    return new IntegerObject { position };
}

Value DirObject::rewind(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    ::rewinddir(m_dir);
    return this;
}

Value DirObject::seek(Env *env, Value position) {
    if (!m_dir) env->raise("IOError", "closed directory");
    long pos = position->as_integer()->to_nat_int_t();
    ::seekdir(m_dir, pos);
    return this;
}

StringObject *DirObject::inspect(Env *env) {
    StringObject *out = new StringObject { "#<" };
    out->append(this->klass()->inspect_str());
    out->append(":");
    out->append(path(env)->as_string());
    out->append(">");
    return out;
}

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
    } else {
        path = fileutil::convert_using_to_path(env, path);
    }

    std::error_code ec;
    auto old_path = std::filesystem::current_path(ec);
    errno = ec.value();
    if (errno) env->raise_errno();

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

Value DirObject::chroot(Env *env, Value path) {
    path = fileutil::convert_using_to_path(env, path);
    auto result = ::chroot(path->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

Value DirObject::mkdir(Env *env, Value path, Value mode) {
    mode_t octmode = 0777;
    if (mode) {
        octmode = IntegerObject::convert_to_int(env, mode);
    }
    path = fileutil::convert_using_to_path(env, path);
    auto result = ::mkdir(path->as_string()->c_str(), octmode);
    if (result < 0) env->raise_errno();
    // need to check dir exists and return nil if mkdir was unsuccessful.
    return Value::integer(0);
}

Value DirObject::pwd(Env *env) {
    return new StringObject { std::filesystem::current_path().c_str() };
}

Value DirObject::rmdir(Env *env, Value path) {
    path = fileutil::convert_using_to_path(env, path);
    auto result = ::rmdir(path->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

Value DirObject::home(Env *env, Value username) {
    if (username) {
        username->assert_type(env, Object::Type::String, "String");
        // lookup home-dir for username
        struct passwd *pw;
        pw = getpwnam(username->as_string()->c_str());
        if (!pw)
            env->raise("ArgumentError", "user {} foobar doesn't exist", username->as_string()->c_str());
        return new StringObject { pw->pw_dir };
    } else {
        // no argument version
        Value home_str = new StringObject { "HOME" };
        Value home_dir = GlobalEnv::the()->Object()->const_fetch("ENV"_s).send(env, "fetch"_s, { home_str });
        return home_dir->dup(env);
    }
}
bool DirObject::is_empty(Env *env, Value dirname) {
    dirname->assert_type(env, Object::Type::String, "String");
    auto dir_cstr = dirname->as_string()->c_str();
    std::error_code ec;
    auto st = std::filesystem::symlink_status(dir_cstr, ec);
    if (ec) {
        errno = ec.value();
        env->raise_errno();
    }
    if (st.type() != std::filesystem::file_type::directory)
        return false;
    const std::filesystem::path dirpath { dir_cstr };
    auto dir_iter = std::filesystem::directory_iterator { dirpath };
    return std::filesystem::begin(dir_iter) == std::filesystem::end(dir_iter);
}
}
