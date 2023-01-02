#include "natalie.hpp"

namespace Natalie {

Value FileStatObject::initialize(Env *env, Value path) {
    if (!path->is_string() && path->respond_to(env, "to_path"_s))
        path = path->send(env, "to_path"_s, { path });

    path->assert_type(env, Object::Type::String, "String");
    if (::stat(path->as_string()->c_str(), &fstatus) != 0)
        env->raise_errno();
    return this;
}

// NATFIXME: Support other file-types
Value FileStatObject::ftype() const {
    if (S_ISREG(fstatus.st_mode)) {
        return new StringObject { "file" };
    }
    return NilObject::the();
}

bool FileStatObject::is_blockdev() const {
    return (S_ISBLK(fstatus.st_mode));
}
bool FileStatObject::is_chardev() const {
    return (S_ISCHR(fstatus.st_mode));
}
bool FileStatObject::is_directory() const {
    return (S_ISDIR(fstatus.st_mode));
}
bool FileStatObject::is_pipe() const {
    return (S_ISFIFO(fstatus.st_mode));
}
bool FileStatObject::is_file() const {
    return (S_ISREG(fstatus.st_mode));
}
bool FileStatObject::is_symlink() const {
    return (S_ISLNK(fstatus.st_mode));
}

Value FileStatObject::is_size() const {
    if (fstatus.st_size == 0)
        return NilObject::the();
    return Value::integer(fstatus.st_size);
}

bool FileStatObject::is_zero() const {
    return (fstatus.st_size == 0);
}

Value FileStatObject::size() const {
    return Value::integer(fstatus.st_size);
}

}
