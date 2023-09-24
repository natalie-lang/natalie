#include "natalie.hpp"

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Natalie {

static inline bool flags_is_readable(const int flags) {
    return (flags & (O_RDONLY | O_WRONLY | O_RDWR)) != O_WRONLY;
}

static inline bool flags_is_writable(const int flags) {
    return (flags & (O_RDONLY | O_WRONLY | O_RDWR)) != O_RDONLY;
}

static inline bool is_readable(const int fd) {
    return flags_is_readable(fcntl(fd, F_GETFL));
}

static inline bool is_writable(const int fd) {
    return flags_is_writable(fcntl(fd, F_GETFL));
}

static void throw_unless_readable(Env *env, const IoObject *const self) {
    // read(2) assigns EBADF to errno if not readable, we want an IOError instead
    const auto old_errno = errno;
    if (!is_readable(self->fileno(env)))
        env->raise("IOError", "not opened for reading");
    errno = old_errno; // errno may have been changed by fcntl, revert to the old value
    env->raise_errno();
}

static void throw_unless_writable(Env *env, const IoObject *const self) {
    // write(2) assigns EBADF to errno if not writable, we want an IOError instead
    const auto old_errno = errno;
    if (!is_writable(self->fileno(env)))
        env->raise("IOError", "not opened for writing");
    errno = old_errno; // errno may have been changed by fcntl, revert to the old value
    env->raise_errno();
}

namespace ioutil {
    // If the `path` is not a string, but has #to_path, then
    // execute #to_path.  Otherwise if it has #to_str, then
    // execute #to_str.  Make sure the path or to_path result is a String
    // before continuing.
    // This is common to many functions in FileObject and DirObject
    StringObject *convert_using_to_path(Env *env, Value path) {
        if (!path->is_string() && path->respond_to(env, "to_path"_s))
            path = path->send(env, "to_path"_s);
        if (!path->is_string() && path->respond_to(env, "to_str"_s))
            path = path->send(env, "to_str"_s);
        path->assert_type(env, Object::Type::String, "String");
        return path->as_string();
    }

    // accepts io or io-like object for fstat
    // accepts path or string like object for stat
    int object_stat(Env *env, Value io, struct stat *sb) {
        if (io->is_io() || io->respond_to(env, "to_io"_s)) {
            if (!io->is_io()) {
                io = io->send(env, "to_io"_s);
            }
            auto file_desc = io->as_io()->fileno();
            return ::fstat(file_desc, sb);
        }

        io = convert_using_to_path(env, io);
        return ::stat(io->as_string()->c_str(), sb);
    }

    flags_struct::flags_struct(Env *env, Value flags_obj) {
        if (!flags_obj || flags_obj->is_nil())
            return;

        if (!flags_obj->is_integer() && !flags_obj->is_string()) {
            if (flags_obj->respond_to(env, "to_str"_s)) {
                flags_obj = flags_obj->to_str(env);
            } else if (flags_obj->respond_to(env, "to_int"_s)) {
                flags_obj = flags_obj->to_int(env);
            }
        }

        switch (flags_obj->type()) {
        case Object::Type::Integer:
            flags = flags_obj->as_integer()->to_nat_int_t();
            break;
        case Object::Type::String: {
            auto colon = new StringObject { ":" };
            auto flagsplit = flags_obj->as_string()->split(env, colon, nullptr)->as_array();
            auto flags_str = flagsplit->fetch(env, IntegerObject::create(static_cast<nat_int_t>(0)), new StringObject { "" }, nullptr)->as_string()->string();
            auto extenc = flagsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(1)), nullptr);
            auto intenc = flagsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(2)), nullptr);
            if (!extenc->is_nil()) external_encoding = EncodingObject::find_encoding(env, extenc);
            if (!intenc->is_nil()) internal_encoding = EncodingObject::find_encoding(env, intenc);

            if (flags_str.length() < 1 || flags_str.length() > 3)
                env->raise("ArgumentError", "invalid access mode {}", flags_str);

            // rb+ => 'r', 'b', '+'
            auto main_mode = flags_str.at(0);
            auto read_write_mode = flags_str.length() > 1 ? flags_str.at(1) : 0;
            auto binary_text_mode = flags_str.length() > 2 ? flags_str.at(2) : 0;

            // rb+ => r+b
            if (read_write_mode == 'b' || read_write_mode == 't')
                std::swap(read_write_mode, binary_text_mode);

            if (binary_text_mode && binary_text_mode != 'b' && binary_text_mode != 't')
                env->raise("ArgumentError", "invalid access mode {}", flags_str);

            if (binary_text_mode == 'b' && !external_encoding) {
                external_encoding = EncodingObject::get(Encoding::ASCII_8BIT);
            } else if (binary_text_mode == 't' && !external_encoding) {
                external_encoding = EncodingObject::get(Encoding::UTF_8);
            }

            if (main_mode == 'r' && !read_write_mode)
                flags = O_RDONLY;
            else if (main_mode == 'r' && read_write_mode == '+')
                flags = O_RDWR;
            else if (main_mode == 'w' && !read_write_mode)
                flags = O_WRONLY | O_CREAT | O_TRUNC;
            else if (main_mode == 'w' && read_write_mode == '+')
                flags = O_RDWR | O_CREAT | O_TRUNC;
            else if (main_mode == 'a' && !read_write_mode)
                flags = O_WRONLY | O_CREAT | O_APPEND;
            else if (main_mode == 'a' && read_write_mode == '+')
                flags = O_RDWR | O_CREAT | O_APPEND;
            else
                env->raise("ArgumentError", "invalid access mode {}", flags_str);
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of {} into String", flags_obj->klass()->inspect_str());
        }
    }

    mode_t perm_to_mode(Env *env, Value perm) {
        if (perm && !perm->is_nil())
            return IntegerObject::convert_to_int(env, perm);
        else
            return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // 0660 default
    }
}

Value IoObject::initialize(Env *env, Value file_number, Value flags_obj) {
    nat_int_t fileno = file_number->to_int(env)->to_nat_int_t();
    assert(fileno >= INT_MIN && fileno <= INT_MAX);
    const auto actual_flags = ::fcntl(fileno, F_GETFL);
    if (actual_flags < 0)
        env->raise_errno();
    if (flags_obj != nullptr && !flags_obj->is_nil()) {
        const ioutil::flags_struct wanted_flags { env, flags_obj };
        if ((flags_is_readable(wanted_flags.flags) && !flags_is_readable(actual_flags)) || (flags_is_writable(wanted_flags.flags) && !flags_is_writable(actual_flags))) {
            errno = EINVAL;
            env->raise_errno();
        }
        set_encoding(env, wanted_flags.external_encoding, wanted_flags.internal_encoding);
    }
    set_fileno(fileno);
    return this;
}

void IoObject::raise_if_closed(Env *env) const {
    if (m_closed) env->raise("IOError", "closed stream");
}

Value IoObject::advise(Env *env, Value advice, Value offset, Value len) {
    raise_if_closed(env);
    advice->assert_type(env, Object::Type::Symbol, "Symbol");
    nat_int_t offset_i = (offset == nullptr) ? 0 : IntegerObject::convert_to_nat_int_t(env, offset);
    nat_int_t len_i = (len == nullptr) ? 0 : IntegerObject::convert_to_nat_int_t(env, len);
    int advice_i = 0;
#ifdef __linux__
    if (advice == "normal"_s) {
        advice_i = POSIX_FADV_NORMAL;
    } else if (advice == "sequential"_s) {
        advice_i = POSIX_FADV_SEQUENTIAL;
    } else if (advice == "random"_s) {
        advice_i = POSIX_FADV_RANDOM;
    } else if (advice == "noreuse"_s) {
        advice_i = POSIX_FADV_NOREUSE;
    } else if (advice == "willneed"_s) {
        advice_i = POSIX_FADV_WILLNEED;
    } else if (advice == "dontneed"_s) {
        advice_i = POSIX_FADV_DONTNEED;
    } else {
        env->raise("NotImplementedError", "Unsupported advice: {}", advice);
    }
    if (::posix_fadvise(m_fileno, offset_i, len_i, advice_i) != 0)
        env->raise_errno();
#else
    if (advice != "normal"_s && advice != "sequential"_s && advice != "random"_s && advice != "noreuse"_s && advice != "willneed"_s && advice != "dontneed"_s) {
        env->raise("NotImplementedError", "Unsupported advice: {}", advice->as_symbol()->string());
    }
#endif
    return NilObject::the();
}

Value IoObject::binread(Env *env, Value filename, Value length, Value offset) {
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename }, nullptr)->as_file();
    if (offset && !offset->is_nil())
        file->set_pos(env, offset);
    file->set_encoding(env, EncodingObject::get(Encoding::ASCII_8BIT));
    auto data = file->read(env, length, nullptr);
    file->close(env);
    return data;
}

Value IoObject::each_byte(Env *env, Block *block) {
    if (block == nullptr)
        return send(env, "enum_for"_s, { "each_byte"_s });

    Value byte;
    while (!(byte = getbyte(env))->is_nil())
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { byte }, nullptr);

    return this;
}

int IoObject::fileno() const {
    return m_fileno;
}

int IoObject::fileno(Env *env) const {
    raise_if_closed(env);
    return m_fileno;
}

Value IoObject::fcntl(Env *env, Value cmd_value, Value arg_value) {
    raise_if_closed(env);
    const auto cmd = IntegerObject::convert_to_int(env, cmd_value);
    int result;
    if (arg_value == nullptr || arg_value->is_nil()) {
        result = ::fcntl(m_fileno, cmd);
    } else if (arg_value->is_string()) {
        const auto arg = arg_value->as_string()->c_str();
        result = ::fcntl(m_fileno, cmd, arg);
    } else {
        const auto arg = IntegerObject::convert_to_int(env, arg_value);
        result = ::fcntl(m_fileno, cmd, arg);
    }
    if (result < 0) env->raise_errno();
    return IntegerObject::create(result);
}

int IoObject::fdatasync(Env *env) {
    raise_if_closed(env);
#ifdef __linux__
    if (::fdatasync(m_fileno) < 0) env->raise_errno();
#endif
    return 0;
}

int IoObject::fsync(Env *env) {
    raise_if_closed(env);
    if (::fsync(m_fileno) < 0) env->raise_errno();
    return 0;
}

Value IoObject::getbyte(Env *env) {
    raise_if_closed(env);
    unsigned char buffer;
    int result = ::read(m_fileno, &buffer, 1);
    if (result == -1) throw_unless_readable(env, this);
    if (result == 0) return NilObject::the(); // eof case
    return Value::integer(buffer);
}

Value IoObject::inspect() const {
    const auto details = m_closed ? TM::String("(closed)") : TM::String::format("fd {}", m_fileno);
    return StringObject::format("#<{}:{}>", klass()->inspect_str(), details);
}

bool IoObject::is_autoclose(Env *env) const {
    raise_if_closed(env);
    return m_autoclose;
}

bool IoObject::is_binmode(Env *env) const {
    raise_if_closed(env);
    return m_external_encoding == EncodingObject::get(Encoding::ASCII_8BIT);
}

bool IoObject::is_eof(Env *env) {
    raise_if_closed(env);
    if (!is_readable(m_fileno))
        env->raise("IOError", "not opened for reading");
    size_t buffer = 0;
    if (::ioctl(m_fileno, FIONREAD, &buffer) < 0)
        env->raise_errno();
    return buffer == 0;
}

bool IoObject::isatty(Env *env) const {
    raise_if_closed(env);
    return ::isatty(m_fileno) == 1;
}

Value IoObject::read_file(Env *env, Value filename, Value length, Value offset) {
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename }, nullptr)->as_file();
    if (offset && !offset->is_nil())
        file->set_pos(env, offset);
    auto data = file->read(env, length, nullptr);
    file->close(env);
    return data;
}

Value IoObject::write_file(Env *env, Value filename, Value string) {
    Value flags = new StringObject { "w" };
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename, flags }, nullptr)->as_file();
    int bytes_written = file->write(env, string);
    file->close(env);
    return Value::integer(bytes_written);
}

#define NAT_READ_BYTES 1024

Value IoObject::read(Env *env, Value count_value, Value buffer) const {
    raise_if_closed(env);
    if (buffer != nullptr && !buffer->is_nil()) {
        if (!buffer->is_string() && buffer->respond_to(env, "to_str"_s))
            buffer = buffer.send(env, "to_str"_s);
        buffer->assert_type(env, Object::Type::String, "String");
    } else {
        buffer = nullptr;
    }
    ssize_t bytes_read;
    if (count_value && !count_value->is_nil()) {
        count_value->assert_type(env, Object::Type::Integer, "Integer");
        int count = count_value->as_integer()->to_nat_int_t();
        if (count < 0)
            env->raise("ArgumentError", "negative length {} given", count);
        TM::String buf(count, '\0');
        bytes_read = ::read(m_fileno, &buf[0], count);
        if (bytes_read < 0)
            env->raise_errno();
        buf.truncate(bytes_read);
        if (bytes_read == 0) {
            if (buffer != nullptr)
                buffer->as_string()->clear(env);
            if (count == 0) {
                if (buffer != nullptr)
                    return buffer;
                return new StringObject { "", 0, EncodingObject::get(Encoding::ASCII_8BIT) };
            }
            return NilObject::the();
        } else if (buffer != nullptr) {
            buffer->as_string()->set_str(buf.c_str(), static_cast<size_t>(bytes_read));
            return buffer;
        } else {
            return new StringObject { std::move(buf), EncodingObject::get(Encoding::ASCII_8BIT) };
        }
    }
    char buf[NAT_READ_BYTES + 1];
    bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
    StringObject *str = nullptr;
    if (buffer != nullptr) {
        str = buffer->as_string();
    } else if (m_external_encoding != nullptr) {
        str = new StringObject { "", m_external_encoding };
    } else {
        str = new StringObject {};
    }
    if (bytes_read < 0) {
        env->raise_errno();
    } else if (bytes_read == 0) {
        str->clear(env);
        return str;
    } else {
        str->set_str(buf, bytes_read);
    }
    while (1) {
        bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
        if (bytes_read < 0) env->raise_errno();
        if (bytes_read == 0) break;
        buf[bytes_read] = 0;
        str->append(buf, bytes_read);
    }
    return str;
}

Value IoObject::append(Env *env, Value obj) {
    raise_if_closed(env);
    obj = obj->to_s(env);
    obj->assert_type(env, Object::Type::String, "String");
    auto result = ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
    if (result == -1) env->raise_errno();
    return this;
}

Value IoObject::autoclose(Env *env, Value value) {
    raise_if_closed(env);
    m_autoclose = value->is_truthy();
    return value;
}

Value IoObject::binmode(Env *env) {
    raise_if_closed(env);
    m_external_encoding = EncodingObject::get(Encoding::ASCII_8BIT);
    m_internal_encoding = nullptr;
    return this;
}

int IoObject::write(Env *env, Value obj) const {
    raise_if_closed(env);
    obj = obj->to_s(env);
    obj->assert_type(env, Object::Type::String, "String");
    int result = ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
    if (result == -1) throw_unless_writable(env, this);
    return result;
}

Value IoObject::write(Env *env, Args args) const {
    args.ensure_argc_at_least(env, 1);
    int bytes_written = 0;
    for (size_t i = 0; i < args.size(); i++) {
        bytes_written += write(env, args[i]);
    }
    return Value::integer(bytes_written);
}

// NATFIXME: Make this spec compliant and maybe more performant?
Value IoObject::gets(Env *env) const {
    raise_if_closed(env);
    char buffer[NAT_READ_BYTES + 1];
    size_t index;
    for (index = 0; index < NAT_READ_BYTES; ++index) {
        if (::read(m_fileno, &buffer[index], 1) == 0)
            return NilObject::the();

        if (buffer[index] == '\n')
            break;
    }
    auto line = new StringObject { buffer, index + 1 };
    env->set_last_line(line);
    return line;
}

Value IoObject::get_path() const {
    if (m_path == nullptr)
        return NilObject::the();
    return new StringObject { *m_path };
}

Value IoObject::pread(Env *env, Value count, Value offset, Value out_string) {
    raise_if_closed(env);
    if (!is_readable(m_fileno))
        env->raise("IOError", "not opened for reading");
    const auto count_int = count->to_int(env)->to_nat_int_t();
    if (count_int < 0)
        env->raise("ArgumentError", "negative string size (or size too big)");
    const auto offset_int = offset->to_int(env)->to_nat_int_t();
    TM::String buf(count_int, '\0');
    const auto bytes_read = ::pread(m_fileno, &buf[0], count_int, offset_int);
    if (bytes_read < 0)
        env->raise_errno();
    if (bytes_read == 0) {
        if (count_int == 0)
            return new StringObject { std::move(buf) };
        env->raise("EOFError", "end of file reached");
    }
    buf.truncate(bytes_read);
    if (out_string != nullptr && !out_string->is_nil()) {
        out_string->assert_type(env, Object::Type::String, "String");
        out_string->as_string()->set_str(buf.c_str(), buf.size());
        return out_string;
    }
    return new StringObject { std::move(buf) };
}

void IoObject::putstr(Env *env, StringObject *str) {
    String sstr = str->string();
    this->send(env, "write"_s, Args({ str }));
    if (sstr.size() == 0 || (sstr.size() > 0 && !sstr.ends_with("\n"))) {
        this->send(env, "write"_s, Args({ new StringObject { "\n" } }));
    }
}

void IoObject::putary(Env *env, ArrayObject *ary) {
    for (auto &item : *ary) {
        this->puts(env, item);
    }
}

void IoObject::puts(Env *env, Value val) {
    if (val->is_string()) {
        this->putstr(env, val->as_string());
    } else if (val->respond_to(env, "to_ary"_s)) {
        auto ary = val->send(env, "to_ary"_s);
        ary->assert_type(env, Object::Type::Array, "Array");
        this->putary(env, ary->as_array());
    } else {
        Value str = val->send(env, "to_s"_s);
        if (str->is_string()) {
            this->putstr(env, str->as_string());
        } else { // to_s did not return a string to inspect val instead.
            this->putstr(env, new StringObject { val->inspect_str(env) });
        }
    }
}

Value IoObject::puts(Env *env, Args args) {
    if (args.size() == 0) {
        this->send(env, "write"_s, Args({ new StringObject { "\n" } }));
    } else {
        for (size_t i = 0; i < args.size(); i++) {
            this->puts(env, args[i]);
        }
    }
    return NilObject::the();
}

Value IoObject::print(Env *env, Args args) const {
    if (args.size() > 0) {
        auto fsep = env->output_file_separator();
        auto valid_fsep = !fsep->is_nil();
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0 && valid_fsep)
                write(env, fsep);
            write(env, args[i]);
        }
    } else {
        Value lastline = env->last_line();
        write(env, lastline);
    }
    auto rsep = env->output_record_separator();
    if (!rsep->is_nil()) write(env, rsep);
    return NilObject::the();
}

Value IoObject::close(Env *env) {
    if (m_closed)
        return NilObject::the();
    int result = ::close(m_fileno);
    if (result == -1) {
        env->raise_errno();
    } else {
        m_closed = true;
        return NilObject::the();
    }
}

Value IoObject::seek(Env *env, Value amount_value, Value whence_value) const {
    raise_if_closed(env);
    nat_int_t amount = IntegerObject::convert_to_nat_int_t(env, amount_value);
    int whence = 0;
    if (whence_value) {
        switch (whence_value->type()) {
        case Object::Type::Integer:
            whence = whence_value->as_integer()->to_nat_int_t();
            break;
        case Object::Type::Symbol: {
            SymbolObject *whence_sym = whence_value->as_symbol();
            if (whence_sym->string() == "SET") {
                whence = 0;
            } else if (whence_sym->string() == "CUR") {
                whence = 1;
            } else if (whence_sym->string() == "END") {
                whence = 2;
            } else {
                env->raise("TypeError", "no implicit conversion of Symbol into Integer");
            }
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of {} into Integer", whence_value->klass()->inspect_str());
        }
    }
    int result = lseek(m_fileno, amount, whence);
    if (result == -1)
        env->raise_errno();
    return Value::integer(0);
}

Value IoObject::set_encoding(Env *env, Value ext_enc, Value int_enc) {
    if ((int_enc == nullptr || int_enc->is_nil()) && ext_enc != nullptr && (ext_enc->is_string() || ext_enc->respond_to(env, "to_str"_s))) {
        ext_enc = ext_enc->to_str(env);
        if (ext_enc->as_string()->include(":")) {
            auto colon = new StringObject { ":" };
            auto encsplit = ext_enc->to_str(env)->split(env, colon, nullptr)->as_array();
            ext_enc = encsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(0)), nullptr);
            int_enc = encsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(1)), nullptr);
        }
    }

    if (ext_enc != nullptr && !ext_enc->is_nil()) {
        if (ext_enc->is_encoding()) {
            m_external_encoding = ext_enc->as_encoding();
        } else {
            m_external_encoding = EncodingObject::find_encoding(env, ext_enc->to_str(env));
        }
    }
    if (int_enc != nullptr && !int_enc->is_nil()) {
        if (int_enc->is_encoding()) {
            m_internal_encoding = int_enc->as_encoding();
        } else {
            m_internal_encoding = EncodingObject::find_encoding(env, int_enc->to_str(env));
        }
    }

    return this;
}

Value IoObject::stat(Env *env) const {
    struct stat sb;
    auto file_desc = fileno(env); // current file descriptor
    int result = ::fstat(file_desc, &sb);
    if (result < 0) env->raise_errno();
    return new FileStatObject { sb };
}

Value IoObject::sysopen(Env *env, Value path, Value flags_obj, Value perm) {
    const ioutil::flags_struct flags { env, flags_obj };
    const auto modenum = ioutil::perm_to_mode(env, perm);

    path = ioutil::convert_using_to_path(env, path);
    const auto fd = ::open(path->as_string()->c_str(), flags.flags, modenum);
    return IntegerObject::create(fd);
}

IoObject *IoObject::to_io(Env *env) {
    return this->as_io();
}

Value IoObject::try_convert(Env *env, Value val) {
    if (val->is_io()) {
        return val;
    } else if (val->respond_to(env, "to_io"_s)) {
        val = val->send(env, "to_io"_s);
        val->assert_type(env, Object::Type::Io, "IO");
        return val;
    }
    return NilObject::the();
}

int IoObject::rewind(Env *env) {
    raise_if_closed(env);
    errno = 0;
    auto result = ::lseek(m_fileno, 0, SEEK_SET);
    if (result < 0 && errno) env->raise_errno();
    return 0;
}

int IoObject::set_pos(Env *env, Value position) {
    raise_if_closed(env);
    nat_int_t offset = IntegerObject::convert_to_nat_int_t(env, position);
    errno = 0;
    auto result = ::lseek(m_fileno, offset, SEEK_SET);
    if (result < 0 && errno) env->raise_errno();
    return result;
}

int IoObject::pos(Env *env) {
    raise_if_closed(env);
    errno = 0;
    auto result = ::lseek(m_fileno, 0, SEEK_CUR);
    if (result < 0 && errno) env->raise_errno();
    return result;
}

// This is a variant of getbyte that raises EOFError
Value IoObject::readbyte(Env *env) {
    auto result = getbyte(env);
    if (result->is_nil())
        env->raise("EOFError", "end of file reached");
    return result;
}

// This is a variant of gets that raises EOFError
// NATFIXME: Add arguments and chomp kwarg when those features are
//  added to IOObject::gets()
Value IoObject::readline(Env *env) const {
    auto result = gets(env);
    if (result->is_nil())
        env->raise("EOFError", "end of file reached");
    return result;
}

}
