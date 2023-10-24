#include "natalie.hpp"
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

Value DirObject::open(Env *env, Value path, Value encoding, Block *block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    if (block) {
        Defer close_dir([&]() {
            dir->as_dir()->close(env);
        });
        Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ dir }), nullptr);
        return result;
    }
    return dir;
}

Value DirObject::initialize(Env *env, Value path, Value encoding) {
    path = ioutil::convert_using_to_path(env, path);
    m_dir = ::opendir(path->as_string()->c_str());
    if (!m_dir) env->raise_errno();
    if (encoding && !encoding->is_nil()) {
        m_encoding = EncodingObject::find_encoding(env, encoding);
    } else {
        m_encoding = EncodingObject::filesystem();
    }
    m_path = path->as_string()->dup(env)->as_string();
    assert(m_path);
    return this;
}

// instance methods
int DirObject::fileno(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    return ::dirfd(m_dir);
    // return new IntegerObject { fdnum };
}

Value DirObject::close(Env *env) {
    if (m_dir)
        ::closedir(m_dir);
    m_dir = nullptr;
    return NilObject::the();
}

Value DirObject::read(Env *env) {
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    dirp = ::readdir(m_dir);
    if (!dirp) return NilObject::the();
    return new StringObject { dirp->d_name, m_encoding };
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
    set_pos(env, position);
    return this;
}

nat_int_t DirObject::set_pos(Env *env, Value position) {
    if (!m_dir) env->raise("IOError", "closed directory");
    nat_int_t pos = position->as_integer()->to_nat_int_t();
    ::seekdir(m_dir, pos);
    return pos;
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
        path = ioutil::convert_using_to_path(env, path);
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
    if (ary->is_empty()) return NilObject::the();
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
    if (ary->is_empty()) return NilObject::the();
    return ary;
}

Value DirObject::each(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { env, this, DirObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each"_s }, size_block);
    }
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    while ((dirp = ::readdir(m_dir))) {
        Value args[] = { new StringObject { dirp->d_name, m_encoding } };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return this;
}

Value DirObject::each_child(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { env, this, DirObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_child"_s }, size_block);
    }
    if (!m_dir) env->raise("IOError", "closed directory");
    struct dirent *dirp;
    while ((dirp = ::readdir(m_dir))) {
        auto name = String(dirp->d_name);
        if (name != "." && name != "..") {
            Value args[] = { new StringObject { name, m_encoding } };
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
        }
    }
    return this;
}

// class method of `children`
Value DirObject::children(Env *env, Value path, Value encoding) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    Defer close_dir([&]() {
        dir->close(env);
    });
    return dir->children(env);
}

// class method of `each_child`
Value DirObject::each_child(Env *env, Value path, Value encoding, Block *block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    auto result = dir->each_child(env, block);
    if (!block) {
        return result;
    }
    return NilObject::the();
}

// class method of `entries`
Value DirObject::entries(Env *env, Value path, Value encoding) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    return dir->entries(env);
}

// class method of `each`
Value DirObject::foreach (Env *env, Value path, Value encoding, Block * block) {
    auto dir = new DirObject {};
    dir->initialize(env, path, encoding);
    auto result = dir->each(env, block);
    if (!block) {
        return result;
    }
    return NilObject::the();
}

Value DirObject::chroot(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    auto result = ::chroot(path->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

Value DirObject::mkdir(Env *env, Value path, Value mode) {
    mode_t octmode = 0777;
    if (mode) {
        octmode = IntegerObject::convert_to_int(env, mode);
    }
    path = ioutil::convert_using_to_path(env, path);
    auto result = ::mkdir(path->as_string()->c_str(), octmode);
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
