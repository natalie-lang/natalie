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

Value FileStatObject::ftype() const {
    const char *ft;
    if (S_ISREG(fstatus.st_mode)) {
        ft = "file";
    } else if (S_ISDIR(fstatus.st_mode)) {
        ft = "directory";
    } else if (S_ISLNK(fstatus.st_mode)) {
        ft = "link";
    } else if (S_ISFIFO(fstatus.st_mode)) {
        ft = "fifo";
    } else if (S_ISBLK(fstatus.st_mode)) {
        ft = "blockSpecial";
    } else if (S_ISCHR(fstatus.st_mode)) {
        ft = "characterSpecial";
    } else if (S_ISSOCK(fstatus.st_mode)) {
        ft = "socket";
    } else {
        ft = "unknown";
    }
    return new StringObject { ft };
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
bool FileStatObject::is_owned() const {
    return (fstatus.st_uid == ::geteuid());
}
bool FileStatObject::is_socket() const {
    return (S_ISSOCK(fstatus.st_mode));
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

bool FileStatObject::is_setuid() const {
    return (fstatus.st_mode & S_ISUID);
}
bool FileStatObject::is_setgid() const {
    return (fstatus.st_mode & S_ISGID);
}
bool FileStatObject::is_sticky() const {
    return (fstatus.st_mode & S_ISVTX);
}

Value FileStatObject::blksize() const {
    return Value::integer(fstatus.st_blksize);
}
Value FileStatObject::blocks() const {
    return Value::integer(fstatus.st_blocks);
}

Value FileStatObject::dev() const {
    return Value::integer(fstatus.st_dev);
}
Value FileStatObject::dev_major() const {
    return Value::integer(major(fstatus.st_dev));
}
Value FileStatObject::dev_minor() const {
    return Value::integer(minor(fstatus.st_dev));
}

Value FileStatObject::ino() const {
    return Value::integer(fstatus.st_ino);
}

Value FileStatObject::mode() const {
    return Value::integer(fstatus.st_mode);
}
Value FileStatObject::nlink() const {
    return Value::integer(fstatus.st_nlink);
}

Value FileStatObject::rdev() const {
    return Value::integer(fstatus.st_rdev);
}
Value FileStatObject::rdev_major() const {
    return Value::integer(major(fstatus.st_rdev));
}
Value FileStatObject::rdev_minor() const {
    return Value::integer(minor(fstatus.st_rdev));
}

Value FileStatObject::size() const {
    return Value::integer(fstatus.st_size);
}

Value FileStatObject::uid() const {
    return Value::integer(fstatus.st_uid);
}
Value FileStatObject::gid() const {
    return Value::integer(fstatus.st_gid);
}

Value FileStatObject::world_readable() const {
    if ((fstatus.st_mode & (S_IROTH)) == S_IROTH) {
        auto modenum = fstatus.st_mode & (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH);
        return Value::integer(modenum);
    }
    return NilObject::the();
}

Value FileStatObject::world_writable() const {
    if ((fstatus.st_mode & (S_IWOTH)) == S_IWOTH) {
        auto modenum = fstatus.st_mode & (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH);
        return Value::integer(modenum);
    }
    return NilObject::the();
}

Value FileStatObject::comparison(Env *env, Value other) const {
    if (other->is_a(env, this->klass()))
        return mtime(env)->as_time()->cmp(env, other->as_file_stat()->mtime(env)->as_time());
    return NilObject::the();
}

Value FileStatObject::atime(Env *env) const {
    Value sec = Value::integer(fstatus.st_atim.tv_sec);
    Value ns = Value::integer(fstatus.st_atim.tv_nsec);
    return TimeObject::at(env, sec, ns, "nanosecond"_s);
}

Value FileStatObject::birthtime(Env *env) const {
#if defined(__FreeBSD__) or defined(__NetBSD__) or defined(__APPLE__)
    Value sec = Value::integer(fstatus.st_birthtimespec.tv_sec);
    Value ns = Value::integer(fstatus.st_birthtimespec.tv_nsec);
    return TimeObject::at(env, sec, ns, "nanosecond"_s);
#else
    env->raise("NotImplementedError", "birthtime not supported on this platform");
#endif
}

Value FileStatObject::ctime(Env *env) const {
    Value sec = Value::integer(fstatus.st_ctim.tv_sec);
    Value ns = Value::integer(fstatus.st_ctim.tv_nsec);
    return TimeObject::at(env, sec, ns, "nanosecond"_s);
}

Value FileStatObject::mtime(Env *env) const {
    Value sec = Value::integer(fstatus.st_mtim.tv_sec);
    Value ns = Value::integer(fstatus.st_mtim.tv_nsec);
    return TimeObject::at(env, sec, ns, "nanosecond"_s);
}

}
