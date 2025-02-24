#include "natalie.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/ioutil.hpp"
#include "natalie/macros.hpp"

#include <errno.h>
#include <filesystem>
#include <sys/stat.h>

namespace Natalie {

DirObject::~DirObject() {
    if (m_dir)
        ::closedir(m_dir);
    m_dir = nullptr;
}

Value DirObject::open(Env *env, Value path, Optional<Value> encoding_kwarg, Block *block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding_kwarg);
    if (block) {
        Defer close_dir([&]() {
            dir->close(env);
        });
        Value result = block->run(env, Args({ dir }), nullptr);
        return result;
    }
    return dir;
}

Value DirObject::initialize(Env *env, Value path, Optional<Value> encoding_kwarg) {
    path = ioutil::convert_using_to_path(env, path);
    m_dir = ::opendir(path.as_string()->c_str());
    if (!m_dir) env->raise_errno();
    if (encoding_kwarg && !encoding_kwarg.value().is_nil()) {
        m_encoding = EncodingObject::find_encoding(env, encoding_kwarg.value());
    } else {
        m_encoding = EncodingObject::filesystem();
    }
    m_path = path.as_string()->duplicate(env).as_string();
    assert(m_path);
    return this;
}

// instance methods
int DirObject::fileno(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    return ::dirfd(m_dir);
}

Value DirObject::close(Env *env) {
    if (m_dir)
        ::closedir(m_dir);
    m_dir = nullptr;
    return Value::nil();
}

Value DirObject::read(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    dirp = ::readdir(m_dir);
    if (!dirp) return Value::nil();
    return new StringObject { dirp->d_name, m_encoding };
}

Value DirObject::tell(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    long position = ::telldir(m_dir);
    return Value::integer(position);
}

Value DirObject::rewind(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    ::rewinddir(m_dir);
    return this;
}

Value DirObject::seek(Env *env, Value position) {
    set_pos(env, position);
    return this;
}

nat_int_t DirObject::set_pos(Env *env, Value position) {
    if (!m_dir) env->raise("IOError", "closed directory");
    nat_int_t pos = position.integer().to_nat_int_t();
    ::seekdir(m_dir, pos);
    return pos;
}

StringObject *DirObject::inspect(Env *env) {
    StringObject *out = new StringObject { "#<" };
    out->append(klass()->inspect_str());
    out->append(":");
    out->append(path(env));
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

Value DirObject::chdir(Env *env, Optional<Value> path_arg, Block *block) {
    auto path = Value::nil();
    if (!path_arg) {
        auto path_str = ::getenv("HOME");
        if (!path_str)
            path_str = ::getenv("LOGDIR");
        if (!path_str)
            env->raise("ArgumentError", "HOME/LOGDIR not set");

        path = new StringObject { path_str };
    } else {
        path = ioutil::convert_using_to_path(env, path_arg.value());
    }

    std::error_code ec;
    auto old_path = std::filesystem::current_path(ec);
    errno = ec.value();
    if (errno) env->raise_errno();

    auto new_path = std::filesystem::path { path.to_str(env)->c_str() };
    change_current_path(env, new_path);

    if (!block)
        return Value::integer(0);

    Value args[] = { path };
    Value result;
    try {
        result = block->run(env, Args(1, args), nullptr);
    } catch (ExceptionObject *exception) {
        change_current_path(env, old_path);
        throw exception;
    }

    change_current_path(env, old_path);
    return result;
}

Value DirObject::children(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    ArrayObject *ary = new ArrayObject {};
    while ((dirp = ::readdir(m_dir))) {
        auto name = String(dirp->d_name);
        if (name != "." && name != "..") {
            ary->push(new StringObject { dirp->d_name, m_encoding });
        }
    }
    if (ary->is_empty()) return Value::nil();
    return ary;
}

// instance method
Value DirObject::entries(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    ArrayObject *ary = new ArrayObject {};
    while ((dirp = ::readdir(m_dir))) {
        ary->push(new StringObject { dirp->d_name, m_encoding });
    }
    if (ary->is_empty()) return Value::nil();
    return ary;
}

Value DirObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, DirObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    while ((dirp = ::readdir(m_dir))) {
        Value args[] = { new StringObject { dirp->d_name, m_encoding } };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value DirObject::each_child(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, DirObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_child"_s }, size_block);
    }
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    while ((dirp = ::readdir(m_dir))) {
        auto name = String(dirp->d_name);
        if (name != "." && name != "..") {
            Value args[] = { new StringObject { name, m_encoding } };
            block->run(env, Args(1, args), nullptr);
        }
    }
    return this;
}

// class method of `children`
Value DirObject::children(Env *env, Value path, Optional<Value> encoding_kwarg) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding_kwarg);
    Defer close_dir([&]() {
        dir->close(env);
    });
    return dir->children(env);
}

// class method of `each_child`
Value DirObject::each_child(Env *env, Value path, Optional<Value> encoding_kwarg, Block *block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding_kwarg);
    auto result = dir->each_child(env, block);
    if (!block) {
        return result;
    }
    return Value::nil();
}

// class method of `entries`
Value DirObject::entries(Env *env, Value path, Optional<Value> encoding_kwarg) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding_kwarg);
    return dir->entries(env);
}

// class method of `each`
Value DirObject::foreach (Env *env, Value path, Optional<Value> encoding_kwarg, Block * block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding_kwarg);
    auto result = dir->each(env, block);
    if (!block) {
        return result;
    }
    return Value::nil();
}

Value DirObject::chroot(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    auto result = ::chroot(path.as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

Value DirObject::mkdir(Env *env, Value path, Optional<Value> mode) {
    mode_t octmode = 0777;
    if (mode)
        octmode = IntegerMethods::convert_to_int(env, mode.value());
    path = ioutil::convert_using_to_path(env, path);
    auto result = ::mkdir(path.as_string()->c_str(), octmode);
    if (result < 0) env->raise_errno();
    // need to check dir exists and return nil if mkdir was unsuccessful.
    return Value::integer(0);
}

Value DirObject::pwd(Env *env) {
    std::error_code ec;
    auto path = std::filesystem::current_path(ec);
    errno = ec.value();
    if (errno)
        env->raise_errno();
    return new StringObject { path.c_str() };
}

Value DirObject::rmdir(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    auto result = ::rmdir(path.as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

Value DirObject::home(Env *env, Optional<Value> username_arg) {
    if (username_arg && !username_arg.value().is_nil()) {
        auto username = username_arg.value();
        username.assert_type(env, Object::Type::String, "String");
        // lookup home-dir for username
        struct passwd *pw;
        pw = getpwnam(username.as_string()->c_str());
        if (!pw)
            env->raise("ArgumentError", "user {} doesn't exist", username.as_string()->c_str());
        return new StringObject { pw->pw_dir };
    } else {
        // no argument version
        Value home_str = new StringObject { "HOME" };
        Value home_dir = GlobalEnv::the()->Object()->const_fetch("ENV"_s).send(env, "[]"_s, { home_str });
        if (!home_dir.is_nil())
            return home_dir->duplicate(env);
        struct passwd *pw;
        pw = getpwuid(getuid());
        assert(pw);
        return new StringObject { pw->pw_dir };
    }
}
bool DirObject::is_empty(Env *env, Value dirname) {
    dirname.assert_type(env, Object::Type::String, "String");
    auto dir_cstr = dirname.as_string()->c_str();
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
