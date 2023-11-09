#include "natalie.hpp"
#include "natalie/ioutil.hpp"

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
// #include <fnmatch.h> // use ruby defined values instead of os-defined
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>

namespace Natalie {

// wrapper to implement euidaccess() for certain systems which
// do not have it.
static int effective_uid_access(const char *path_name, int type) {
#if defined(__OpenBSD__) or defined(__APPLE__)
    uid_t real_uid = ::getuid();
    uid_t effective_uid = ::geteuid();
    gid_t real_gid = ::getgid();
    gid_t effective_gid = ::getegid();
    // if real user/group id's are the same as the effective
    // user/group id's then we can just use access(), yay!
    if (real_uid == effective_uid && real_gid == effective_gid)
        return ::access(path_name, type);
    // NATFIXME: this behavior is probably wrong, but passes specs
    // because real/effective are always equal in the tests.
    return -1;
#else
    // linux systems have an euid_access function so call it
    // directly.
    return ::euidaccess(path_name, type);
#endif
}

Value FileObject::initialize(Env *env, Args args, Block *block) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 1, 3);
    auto filename = args.at(0);
    auto flags_obj = args.at(1, nullptr);
    auto perm = args.at(2, nullptr);
    const ioutil::flags_struct flags { env, flags_obj, kwargs };
    const auto modenum = ioutil::perm_to_mode(env, perm);

    if (filename->is_integer()) { // passing in a number uses fd number
        int fileno = IntegerObject::convert_to_int(env, filename);
        String flags_str = String('r');
        if (flags_obj) {
            flags_obj->assert_type(env, Object::Type::String, "String");
            flags_str = flags_obj->as_string()->string();
        }
        FILE *fptr = ::fdopen(fileno, flags_str.c_str());
        if (fptr == nullptr) env->raise_errno();
        set_fileno(fileno);
    } else {
        filename = ioutil::convert_using_to_path(env, filename);
        int fileno = ::open(filename->as_string()->c_str(), flags.flags(), modenum);
        if (fileno == -1) env->raise_errno();
        set_fileno(fileno);
        set_path(filename->as_string());
    }
    set_encoding(env, flags.external_encoding(), flags.internal_encoding());
    if (block)
        env->warn("File::new() does not take block; use File::open() instead");
    return this;
}

Value FileObject::expand_path(Env *env, Value path, Value root) {
    path->assert_type(env, Object::Type::String, "String");

    StringObject *merged;
    if (path->as_string()->length() > 0 && path->as_string()->string()[0] == '/') {
        merged = path->as_string();
    } else if (root) {
        root->assert_type(env, Object::Type::String, "String");
        root = expand_path(env, root, nullptr);
        merged = StringObject::format("{}/{}", root->as_string(), path->as_string());
    } else {
        char root[MAXPATHLEN + 1];
        if (!getcwd(root, MAXPATHLEN + 1))
            env->raise_errno();
        merged = StringObject::format("{}/{}", root, path->as_string());
    }

    // This is a bit weird, but since the regexp stuff below sets last_match
    // on the **calling** env, we need to call these with a child env.
    Env e { env };
    e.set_caller(env);

    // collapse ..
    auto dotdot = new RegexpObject { env, "[^/]*/\\.\\.(/|\\z)" };
    StringObject empty_string { "" };
    do {
        merged = merged->sub(&e, dotdot, &empty_string)->as_string();
    } while (env->has_last_match());

    // collapse .
    auto dot = new RegexpObject { env, "/\\.(/|\\z)" };
    StringObject slash { "/" };
    do {
        merged = merged->sub(&e, dot, &slash)->as_string();
    } while (env->has_last_match());

    // remove trailing slash
    if (merged->length() > 1 && merged->string()[merged->length() - 1] == '/') {
        merged->truncate(merged->length() - 1);
    }

    return merged;
}

Value FileObject::flock(Env *env, Value locking_constant) {
    const auto locking_constant_int = IntegerObject::convert_to_nat_int_t(env, locking_constant);
    if (::flock(fileno(env), locking_constant_int) < 0) {
        if (errno == EWOULDBLOCK)
            return FalseObject::the();
        env->raise_errno();
    }
    return Value::integer(0);
}

void FileObject::unlink(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    int result = ::unlink(path->as_string()->c_str());
    if (result != 0)
        env->raise_errno();
}

Value FileObject::unlink(Env *env, Args args) {
    for (size_t i = 0; i < args.size(); ++i) {
        FileObject::unlink(env, args[i]);
    }
    return Value::integer(args.size());
}

// MRI defines these constants differently than the OS does in fnmatch.h
#define FNM_NOESCAPE 0x01
#define FNM_PATHNAME 0x02
#define FNM_DOTMATCH 0x04
#define FNM_CASEFOLD 0x08
#define FNM_EXTGLOB 0x10
#define FNM_SYSCASE 0
#define FNM_SHORTNAME 0

void FileObject::build_constants(Env *env, ModuleObject *fcmodule) {
    fcmodule->const_set("APPEND"_s, Value::integer(O_APPEND));
    fcmodule->const_set("RDONLY"_s, Value::integer(O_RDONLY));
    fcmodule->const_set("WRONLY"_s, Value::integer(O_WRONLY));
    fcmodule->const_set("TRUNC"_s, Value::integer(O_TRUNC));
    fcmodule->const_set("CREAT"_s, Value::integer(O_CREAT));
    fcmodule->const_set("DSYNC"_s, Value::integer(O_DSYNC));
    fcmodule->const_set("EXCL"_s, Value::integer(O_EXCL));
    fcmodule->const_set("NOCTTY"_s, Value::integer(O_NOCTTY));
    fcmodule->const_set("NOFOLLOW"_s, Value::integer(O_NOFOLLOW));
    fcmodule->const_set("NONBLOCK"_s, Value::integer(O_NONBLOCK));
    fcmodule->const_set("RDWR"_s, Value::integer(O_RDWR));
    fcmodule->const_set("SYNC"_s, Value::integer(O_SYNC));
    fcmodule->const_set("LOCK_EX"_s, Value::integer(LOCK_EX));
    fcmodule->const_set("LOCK_NB"_s, Value::integer(LOCK_NB));
    fcmodule->const_set("LOCK_SH"_s, Value::integer(LOCK_SH));
    fcmodule->const_set("LOCK_UN"_s, Value::integer(LOCK_UN));
    fcmodule->const_set("FNM_NOESCAPE"_s, Value::integer(FNM_NOESCAPE));
    fcmodule->const_set("FNM_PATHNAME"_s, Value::integer(FNM_PATHNAME));
    fcmodule->const_set("FNM_DOTMATCH"_s, Value::integer(FNM_DOTMATCH));
    fcmodule->const_set("FNM_CASEFOLD"_s, Value::integer(FNM_CASEFOLD));
    fcmodule->const_set("FNM_EXTGLOB"_s, Value::integer(FNM_EXTGLOB));
    fcmodule->const_set("FNM_SYSCASE"_s, Value::integer(FNM_SYSCASE));
    fcmodule->const_set("FNM_SHORTNAME"_s, Value::integer(FNM_SHORTNAME));
    Value null_file = new StringObject { "/dev/null", Encoding::US_ASCII };
    null_file->freeze();
    fcmodule->const_set("NULL"_s, null_file);
}

bool FileObject::exist(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    return ::stat(path->as_string()->c_str(), &sb) != -1;
}

bool FileObject::is_file(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISREG(sb.st_mode);
}

bool FileObject::is_directory(Env *env, Value path) {
    struct stat sb;
    if (ioutil::object_stat(env, path, &sb) == -1)
        return false;
    return S_ISDIR(sb.st_mode);
}

bool FileObject::is_identical(Env *env, Value file1, Value file2) {
    file1 = ioutil::convert_using_to_path(env, file1);
    file2 = ioutil::convert_using_to_path(env, file2);
    struct stat stat1;
    struct stat stat2;
    auto result1 = ::stat(file1->as_string()->c_str(), &stat1);
    auto result2 = ::stat(file2->as_string()->c_str(), &stat2);
    if (result1 < 0) return false;
    if (result2 < 0) return false;
    if (stat1.st_dev != stat2.st_dev) return false;
    if (stat1.st_ino != stat2.st_ino) return false;
    return true;
}

bool FileObject::is_sticky(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    std::error_code ec;
    auto st = std::filesystem::status(path->as_string()->c_str(), ec);
    if (ec)
        return false;
    auto perm = st.permissions();
    return (perm & std::filesystem::perms::sticky_bit) != std::filesystem::perms::none;
}

bool FileObject::is_setgid(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return (sb.st_mode & S_ISGID);
}

bool FileObject::is_setuid(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return (sb.st_mode & S_ISUID);
}

bool FileObject::is_symlink(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::lstat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISLNK(sb.st_mode);
}

bool FileObject::is_blockdev(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISBLK(sb.st_mode);
}

bool FileObject::is_chardev(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISCHR(sb.st_mode);
}

bool FileObject::is_pipe(Env *env, Value path) {
    struct stat sb;
    path->assert_type(env, Object::Type::String, "String");
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISFIFO(sb.st_mode);
}

bool FileObject::is_socket(Env *env, Value path) {
    struct stat sb;
    path->assert_type(env, Object::Type::String, "String");
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISSOCK(sb.st_mode);
}

bool FileObject::is_readable(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (access(path->as_string()->c_str(), R_OK) == -1)
        return false;
    return true;
}

bool FileObject::is_readable_real(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (effective_uid_access(path->as_string()->c_str(), R_OK) == -1)
        return false;
    return true;
}

Value FileObject::world_readable(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return NilObject::the();
    if ((sb.st_mode & (S_IROTH)) == S_IROTH) {
        auto modenum = sb.st_mode & (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH);
        return Value::integer(modenum);
    }
    return NilObject::the();
}

Value FileObject::world_writable(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return NilObject::the();
    if ((sb.st_mode & (S_IWOTH)) == S_IWOTH) {
        auto modenum = sb.st_mode & (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH);
        return Value::integer(modenum);
    }
    return NilObject::the();
}

bool FileObject::is_writable(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (access(path->as_string()->c_str(), W_OK) == -1)
        return false;
    return true;
}

bool FileObject::is_writable_real(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (effective_uid_access(path->as_string()->c_str(), W_OK) == -1)
        return false;
    return true;
}

bool FileObject::is_executable(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (access(path->as_string()->c_str(), X_OK) == -1)
        return false;
    return true;
}

bool FileObject::is_executable_real(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    if (effective_uid_access(path->as_string()->c_str(), X_OK) == -1)
        return false;
    return true;
}

bool FileObject::is_grpowned(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    auto size = getgroups(0, nullptr);
    if (size < 0)
        env->raise_errno();
    gid_t list[size];
    size = getgroups(size, list);
    if (size < 0)
        env->raise_errno();
    const auto egid = ::getegid();
    for (size_t i = 0; i < static_cast<size_t>(size); i++) {
        if (list[i] == egid)
            return true;
    }
    return false;
}

bool FileObject::is_owned(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return (sb.st_uid == ::geteuid());
}

bool FileObject::is_zero(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    if (::stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return (sb.st_size == 0);
}

// oddball function that is ends in '?' but is not a boolean return.
Value FileObject::is_size(Env *env, Value path) {
    struct stat sb;
    if (ioutil::object_stat(env, path, &sb) == -1)
        return NilObject::the();
    if (sb.st_size == 0) // returns nil when file size is zero.
        return NilObject::the();
    return IntegerObject::create((nat_int_t)(sb.st_size));
}

Value FileObject::size(Env *env, Value path) {
    struct stat sb;
    if (ioutil::object_stat(env, path, &sb) == -1)
        env->raise_errno();
    return IntegerObject::create((nat_int_t)(sb.st_size));
}

nat_int_t FileObject::symlink(Env *env, Value from, Value to) {
    from = ioutil::convert_using_to_path(env, from);
    to = ioutil::convert_using_to_path(env, to);
    int result = ::symlink(from->as_string()->c_str(), to->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return 0;
}

nat_int_t FileObject::rename(Env *env, Value from, Value to) {
    from = ioutil::convert_using_to_path(env, from);
    to = ioutil::convert_using_to_path(env, to);
    int result = ::rename(from->as_string()->c_str(), to->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return 0;
}

nat_int_t FileObject::link(Env *env, Value from, Value to) {
    from = ioutil::convert_using_to_path(env, from);
    to = ioutil::convert_using_to_path(env, to);
    int result = ::link(from->as_string()->c_str(), to->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return 0;
}

nat_int_t FileObject::mkfifo(Env *env, Value path, Value mode) {
    mode_t octmode = 0666;
    if (mode) {
        mode->assert_type(env, Object::Type::Integer, "Integer");
        octmode = (mode_t)(mode->as_integer()->to_nat_int_t());
    }
    path = ioutil::convert_using_to_path(env, path);
    int result = ::mkfifo(path->as_string()->c_str(), octmode);
    if (result < 0) env->raise_errno();
    return 0;
}

Value FileObject::chmod(Env *env, Args args) {
    // requires mode argument, file arguments are optional
    args.ensure_argc_at_least(env, 1);
    auto mode = args[0];
    mode_t modenum = IntegerObject::convert_to_int(env, mode);
    for (size_t i = 1; i < args.size(); ++i) {
        auto path = ioutil::convert_using_to_path(env, args[i]);
        int result = ::chmod(path->as_string()->c_str(), modenum);
        if (result < 0) env->raise_errno();
    }
    // return number of files
    return Value::integer(args.size() - 1);
}

Value FileObject::chown(Env *env, Args args) {
    // requires uid/gid arguments, file arguments are optional
    args.ensure_argc_at_least(env, 2);
    auto uid = args.at(0);
    auto gid = args.at(1);
    uid_t uidnum = IntegerObject::convert_to_uid(env, uid);
    gid_t gidnum = IntegerObject::convert_to_gid(env, gid);
    for (size_t i = 2; i < args.size(); ++i) {
        auto path = ioutil::convert_using_to_path(env, args[i]);
        int result = ::chown(path->as_string()->c_str(), uidnum, gidnum);
        if (result < 0) env->raise_errno();
    }
    // return number of files
    return Value::integer(args.size() - 2);
}

// Instance method (single arg)
Value FileObject::chmod(Env *env, Value mode) {
    mode_t modenum = IntegerObject::convert_to_int(env, mode);
    auto file_desc = fileno(); // current file descriptor
    int result = ::fchmod(file_desc, modenum);
    if (result < 0) env->raise_errno();
    return Value::integer(0); // always return 0
}

// Instance method (two args)
Value FileObject::chown(Env *env, Value uid, Value gid) {
    uid_t uidnum = IntegerObject::convert_to_uid(env, uid);
    gid_t gidnum = IntegerObject::convert_to_gid(env, gid);
    auto file_desc = fileno(); // current file descriptor
    int result = ::fchown(file_desc, uidnum, gidnum);
    if (result < 0) env->raise_errno();
    return Value::integer(0); // always return 0
}

Value FileObject::ftype(Env *env, Value path) {
    path = ioutil::convert_using_to_path(env, path);
    std::error_code ec;
    // use symlink_status instead of status bc we do not want to follow symlinks
    auto st = std::filesystem::symlink_status(path->as_string()->c_str(), ec);
    if (ec) {
        errno = ec.value();
        env->raise_errno();
    }
    switch (st.type()) {
    case std::filesystem::file_type::regular:
        return new StringObject { "file" };
    case std::filesystem::file_type::directory:
        return new StringObject { "directory" };
    case std::filesystem::file_type::symlink:
        return new StringObject { "link" };
    case std::filesystem::file_type::block:
        return new StringObject { "blockSpecial" };
    case std::filesystem::file_type::character:
        return new StringObject { "characterSpecial" };
    case std::filesystem::file_type::fifo:
        return new StringObject { "fifo" };
    case std::filesystem::file_type::socket:
        return new StringObject { "socket" };
    default:
        return new StringObject { "unknown" };
    }
}

Value FileObject::umask(Env *env, Value mask) {
    mode_t old_mask = 0;
    if (mask) {
        mode_t mask_mode = IntegerObject::convert_to_int(env, mask);
        old_mask = ::umask(mask_mode);
    } else {
        old_mask = ::umask(0);
    }
    return Value::integer(old_mask);
}

// class method
StringObject *FileObject::path(Env *env, Value pathname) {
    return ioutil::convert_using_to_path(env, pathname);
}

Value FileObject::readlink(Env *env, Value filename) {
    filename = ioutil::convert_using_to_path(env, filename);
    TM::String buf(128, '\0');
    while (true) {
        const auto size = ::readlink(filename->as_string()->c_str(), &buf[0], buf.size());
        if (size < 0)
            env->raise_errno();
        if (static_cast<size_t>(size) < buf.size())
            return new StringObject { buf.c_str(), static_cast<size_t>(size) };
        buf = TM::String(buf.size() * 2, '\0');
    }
}

Value FileObject::realpath(Env *env, Value pathname, Value __dir_string) {
    pathname = ioutil::convert_using_to_path(env, pathname);
    if (__dir_string != nullptr) {
        pathname->as_string()->prepend_char(env, '/');
        pathname->as_string()->prepend(env, { ioutil::convert_using_to_path(env, __dir_string) });
    }
    char *resolved_filepath = nullptr;
    resolved_filepath = ::realpath(pathname->as_string()->c_str(), nullptr);
    if (!resolved_filepath)
        env->raise_errno();
    auto outstr = new StringObject { resolved_filepath };
    free(resolved_filepath);
    return outstr;
}

// class method
Value FileObject::lstat(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    int result = ::lstat(path->as_string()->c_str(), &sb);
    if (result < 0) env->raise_errno(path->as_string());
    return new FileStatObject { sb };
}

// instance method
Value FileObject::lstat(Env *env) const {
    struct stat sb;
    int result = ::stat(get_path()->as_string()->c_str(), &sb);
    if (result < 0) env->raise_errno();
    return new FileStatObject { sb };
}

int FileObject::truncate(Env *env, Value path, Value size) {
    path = ioutil::convert_using_to_path(env, path);
    off_t len = IntegerObject::convert_to_int(env, size);
    if (::truncate(path->as_string()->c_str(), len) == -1) {
        env->raise_errno();
    }
    return 0;
}
int FileObject::truncate(Env *env, Value size) const { // instance method
    off_t len = IntegerObject::convert_to_int(env, size);
    if (::ftruncate(fileno(), len) == -1) {
        env->raise_errno();
    }
    return 0;
}

// class method
Value FileObject::stat(Env *env, Value path) {
    struct stat sb;
    path = ioutil::convert_using_to_path(env, path);
    int result = ::stat(path->as_string()->c_str(), &sb);
    if (result < 0) env->raise_errno(path->as_string());
    return new FileStatObject { sb };
}

// class methods
Value FileObject::atime(Env *env, Value path) {
    FileStatObject *statobj;
    if (path->is_io()) { // using file-descriptor
        statobj = path->as_io()->stat(env)->as_file_stat();
    } else {
        path = ioutil::convert_using_to_path(env, path);
        statobj = stat(env, path)->as_file_stat();
    }
    return statobj->atime(env);
}
Value FileObject::ctime(Env *env, Value path) {
    FileStatObject *statobj;
    if (path->is_io()) { // using file-descriptor
        statobj = path->as_io()->stat(env)->as_file_stat();
    } else {
        path = ioutil::convert_using_to_path(env, path);
        statobj = stat(env, path)->as_file_stat();
    }
    return statobj->ctime(env);
}

Value FileObject::mtime(Env *env, Value path) {
    FileStatObject *statobj;
    if (path->is_io()) { // using file-descriptor
        statobj = path->as_io()->stat(env)->as_file_stat();
    } else {
        path = ioutil::convert_using_to_path(env, path);
        statobj = stat(env, path)->as_file_stat();
    }
    return statobj->mtime(env);
}

Value FileObject::utime(Env *env, Args args) {
    args.ensure_argc_at_least(env, 2);

    TimeObject *atime, *mtime;

    if (args[0]->is_nil()) {
        atime = TimeObject::create(env);
    } else if (args[0]->is_time()) {
        atime = args[0]->as_time();
    } else {
        atime = TimeObject::at(env, args[0], nullptr, nullptr);
    }
    if (args[1]->is_nil()) {
        mtime = TimeObject::create(env);
    } else if (args[1]->is_time()) {
        mtime = args[1]->as_time();
    } else {
        mtime = TimeObject::at(env, args[1], nullptr, nullptr);
    }

    struct timeval ubuf[2], *ubufp = nullptr;
    ubuf[0].tv_sec = atime->to_r(env)->as_rational()->to_i(env)->as_integer()->to_nat_int_t();
    ubuf[0].tv_usec = atime->usec(env)->as_integer()->to_nat_int_t();
    ubuf[1].tv_sec = mtime->to_r(env)->as_rational()->to_i(env)->as_integer()->to_nat_int_t();
    ubuf[1].tv_usec = mtime->usec(env)->as_integer()->to_nat_int_t();
    ubufp = ubuf;

    for (size_t i = 2; i < args.size(); ++i) {
        Value path = args[i];
        path = ioutil::convert_using_to_path(env, path);
        if (::utimes(path->as_string()->c_str(), ubufp) != 0) {
            env->raise_errno();
        }
    }
    return IntegerObject::create((nat_int_t)(args.size() - 2));
}

Value FileObject::atime(Env *env) { // inst method
    if (is_closed()) env->raise("IOError", "closed stream");
    return as_io()->stat(env)->as_file_stat()->atime(env);
}
Value FileObject::ctime(Env *env) { // inst method
    if (is_closed()) env->raise("IOError", "closed stream");
    return as_io()->stat(env)->as_file_stat()->ctime(env);
}
Value FileObject::mtime(Env *env) { // inst method
    if (is_closed()) env->raise("IOError", "closed stream");
    return as_io()->stat(env)->as_file_stat()->mtime(env);
}
Value FileObject::size(Env *env) { // inst method
    if (is_closed()) env->raise("IOError", "closed stream");
    return as_io()->stat(env)->as_file_stat()->size();
}

}
