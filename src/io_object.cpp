#include "natalie.hpp"
#include "natalie/ioutil.hpp"

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/select.h>
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

Value IoObject::initialize(Env *env, Args args, Block *block) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 1, 2);
    Value file_number = args.at(0);
    Value flags_obj = args.at(1, nullptr);
    const ioutil::flags_struct wanted_flags { env, flags_obj, kwargs };
    nat_int_t fileno = file_number->to_int(env)->to_nat_int_t();
    assert(fileno >= INT_MIN && fileno <= INT_MAX);
    const auto actual_flags = ::fcntl(fileno, F_GETFL);
    if (actual_flags < 0)
        env->raise_errno();
    if (wanted_flags.has_mode) {
        if ((flags_is_readable(wanted_flags.flags) && !flags_is_readable(actual_flags)) || (flags_is_writable(wanted_flags.flags) && !flags_is_writable(actual_flags))) {
            errno = EINVAL;
            env->raise_errno();
        }
    }
    set_fileno(fileno);
    set_encoding(env, wanted_flags.external_encoding, wanted_flags.internal_encoding);
    m_autoclose = wanted_flags.autoclose;
    if (block)
        env->warn("{}::new() does not take block; use {}::open() instead", m_klass->inspect_str(), m_klass->inspect_str());
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

Value IoObject::binwrite(Env *env, Args args) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 2, 3);
    auto filename = args.at(0);
    auto string = args.at(1);
    auto offset = args.at(2, nullptr);
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    auto mode = O_WRONLY | O_CREAT | O_CLOEXEC;
    if (!offset || offset->is_nil())
        mode |= O_TRUNC;
    if (kwargs) {
        kwargs = kwargs->dup(env)->as_hash();
    } else {
        kwargs = new HashObject {};
    }
    if (!kwargs->has_key(env, "mode"_s))
        kwargs->put(env, "mode"_s, IntegerObject::create(mode));
    kwargs->put(env, "binmode"_s, TrueObject::the());
    FileObject *file = _new(env, File, Args({ filename, kwargs }, true), nullptr)->as_file();
    if (offset && !offset->is_nil())
        file->set_pos(env, offset);
    auto result = file->write(env, string);
    file->close(env);
    return IntegerObject::create(result);
}

Value IoObject::dup(Env *env) const {
    auto dup_fd = ::dup(fileno(env));
    if (dup_fd < 0)
        env->raise_errno();
    auto dup_obj = _new(env, m_klass, { IntegerObject::create(dup_fd) }, nullptr)->as_io();
    dup_obj->set_close_on_exec(env, TrueObject::the());
    dup_obj->autoclose(env, TrueObject::the());
    return dup_obj;
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

bool IoObject::is_close_on_exec(Env *env) const {
    raise_if_closed(env);
    int flags = ::fcntl(m_fileno, F_GETFD);
    if (flags < 0)
        env->raise_errno();
    return flags & FD_CLOEXEC;
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

int IoObject::lineno(Env *env) const {
    raise_if_closed(env);
    if (!is_readable(m_fileno))
        env->raise("IOError", "not opened for reading");
    return m_lineno;
}

Value IoObject::read_file(Env *env, Args args) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 1, 3);
    auto filename = args.at(0);
    auto length = args.at(1, nullptr);
    auto offset = args.at(2, nullptr);
    const ioutil::flags_struct flags { env, nullptr, kwargs };
    if (!flags_is_readable(flags.flags))
        env->raise("IOError", "not opened for reading");
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename }, nullptr)->as_file();
    file->set_encoding(env, flags.external_encoding, flags.internal_encoding);
    if (offset && !offset->is_nil())
        file->set_pos(env, offset);
    auto data = file->read(env, length, nullptr);
    file->close(env);
    return data;
}

Value IoObject::write_file(Env *env, Args args) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_is(env, 2);
    auto filename = args.at(0);
    auto string = args.at(1);
    Value mode = new StringObject { "w" };
    if (kwargs && kwargs->has_key(env, "mode"_s))
        mode = kwargs->delete_key(env, "mode"_s, nullptr);
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, Args({ filename, mode, kwargs }, true), nullptr)->as_file();
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
        auto count_value_integer = count_value->as_integer();
        if (!count_value_integer->is_fixnum())
            env->raise("RangeError", "bignum too big to convert into `long'");
        long count = count_value->as_integer()->to_nat_int_t();
        if (count < 0)
            env->raise("ArgumentError", "negative length {} given", (long long)count);
        if (count > std::numeric_limits<off_t>::max())
            env->raise("RangeError", "bignum too big to convert into `long'");
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
    if (m_sync) ::fsync(m_fileno);
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
    if (m_sync) ::fsync(m_fileno);
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

// NATFIXME: Make this spec compliant
Value IoObject::gets(Env *env, Value chomp) {
    raise_if_closed(env);
    auto line = new StringObject {};
    char buffer[NAT_READ_BYTES + 1];

    errno = 0;
    auto pos = ::lseek(m_fileno, 0, SEEK_CUR);
    if (pos < 0 && errno)
        env->raise_errno();

    while (true) {
        const auto bytes_read = ::pread(m_fileno, buffer, NAT_READ_BYTES, pos + line->bytesize());
        if (bytes_read < 0)
            env->raise_errno();
        if (bytes_read == 0)
            break;
        line->append(buffer, bytes_read);
        if (line->include("\n"))
            break;
    }
    if (line->is_empty()) {
        env->set_last_line(NilObject::the());
        return NilObject::the();
    }
    const auto *ptr = strstr(line->c_str(), "\n");
    if (ptr)
        line->truncate(ptr - line->c_str() + 1);
    errno = 0;
    auto new_pos = ::lseek(m_fileno, line->bytesize(), SEEK_CUR);
    if (new_pos < 0 && errno)
        env->raise_errno();
    if (chomp && chomp->is_truthy())
        line->chomp_in_place(env, nullptr);
    m_lineno++;
    env->set_last_line(line);
    env->set_last_lineno(IntegerObject::create(m_lineno));
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
        if (!out_string->is_string())
            out_string = out_string->to_str(env);
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

Value IoObject::pwrite(Env *env, Value data, Value offset) {
    raise_if_closed(env);
    if (!is_writable(m_fileno))
        env->raise("IOError", "not opened for writing");
    auto offset_int = IntegerObject::convert_to_nat_int_t(env, offset);
    auto str = data->to_s(env);
    auto result = ::pwrite(m_fileno, str->c_str(), str->bytesize(), offset_int);
    if (result < 0)
        env->raise_errno();
    return IntegerObject::create(result);
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

Value IoObject::set_close_on_exec(Env *env, Value value) {
    raise_if_closed(env);
    int flags = ::fcntl(m_fileno, F_GETFD);
    if (flags < 0)
        env->raise_errno();
    if (value->is_truthy()) {
        flags |= FD_CLOEXEC;
    } else {
        flags &= ~FD_CLOEXEC;
    }
    if (::fcntl(m_fileno, F_SETFD, flags) < 0)
        env->raise_errno();
    return value;
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

Value IoObject::set_lineno(Env *env, Value lineno) {
    raise_if_closed(env);
    if (!is_readable(m_fileno))
        env->raise("IOError", "not opened for reading");
    m_lineno = IntegerObject::convert_to_int(env, lineno);
    return lineno;
}

Value IoObject::set_sync(Env *env, Value value) {
    raise_if_closed(env);
    m_sync = value->is_truthy();
    return value;
}

Value IoObject::stat(Env *env) const {
    struct stat sb;
    auto file_desc = fileno(env); // current file descriptor
    int result = ::fstat(file_desc, &sb);
    if (result < 0) env->raise_errno();
    return new FileStatObject { sb };
}

Value IoObject::sysopen(Env *env, Value path, Value flags_obj, Value perm) {
    const ioutil::flags_struct flags { env, flags_obj, nullptr };
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
        auto io = val->send(env, "to_io"_s);
        if (!io->is_io())
            env->raise(
                "TypeError", "can't convert {} to IO ({}#to_io gives {})",
                val->klass()->inspect_str(),
                val->klass()->inspect_str(),
                io->klass()->inspect_str());
        return io;
    }
    return NilObject::the();
}

int IoObject::rewind(Env *env) {
    raise_if_closed(env);
    errno = 0;
    auto result = ::lseek(m_fileno, 0, SEEK_SET);
    if (result < 0 && errno) env->raise_errno();
    m_lineno = 0;
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

bool IoObject::sync(Env *env) const {
    raise_if_closed(env);
    return m_sync;
}
Value IoObject::select(Env *env, Value read_ios, Value write_ios, Value error_ios, Value timeout) {
    int nfds = 0;
    timeval timeout_tv = { 0, 0 }, *timeout_ptr = nullptr;

    auto create_fd_set = [&](Value ios) {
        fd_set result;
        FD_ZERO(&result);
        if (ios && !ios->is_nil()) {
            for (Value io : *ios->to_ary(env)) {
                const auto fd = io->to_io(env)->fileno();
                FD_SET(fd, &result);
                nfds = std::max(nfds, fd + 1);
            }
        }
        return result;
    };

    auto read_fds = create_fd_set(read_ios);
    auto write_fds = create_fd_set(write_ios);
    auto error_fds = create_fd_set(error_ios);

    if (timeout && !timeout->is_nil()) {
        const auto timeout_f = timeout->to_f(env)->to_double();
        if (timeout_f < 0)
            env->raise("ArgumentError", "time interval must not be negative");
        timeout_tv.tv_sec = static_cast<int>(timeout_f);
        timeout_tv.tv_usec = (timeout_f - timeout_tv.tv_sec) * 1000000;
        timeout_ptr = &timeout_tv;
    }

    const auto result = ::select(nfds, &read_fds, &write_fds, &error_fds, timeout_ptr);
    if (result < 0)
        env->raise_errno();
    if (result == 0)
        return NilObject::the();

    auto create_output_fds = [&](fd_set *fds, Value ios) {
        auto result = new ArrayObject {};
        if (ios && !ios->is_nil()) {
            for (auto io : *ios->to_ary(env)) {
                if (FD_ISSET(io->to_io(env)->fileno(), fds))
                    result->push(io);
            }
        }
        return result;
    };

    auto readable_ios = create_output_fds(&read_fds, read_ios);
    auto writeable_ios = create_output_fds(&write_fds, write_ios);
    auto errorable_ios = create_output_fds(&error_fds, error_ios);
    return new ArrayObject { readable_ios, writeable_ios, errorable_ios };
}

Value IoObject::pipe(Env *env, Value external_encoding, Value internal_encoding, Block *block, ClassObject *klass) {
    int pipefd[2];
#ifdef __APPLE__
    // No pipe2, use pipe and set permissions afterwards
    if (::pipe(pipefd) < 0)
        env->raise_errno();
    const auto read_flags = ::fcntl(pipefd[0], F_GETFD);
    if (read_flags < 0)
        env->raise_errno();
    if (::fcntl(pipefd[0], F_SETFD, read_flags | O_CLOEXEC | O_NONBLOCK) < 0)
        env->raise_errno();
    const auto write_flags = ::fcntl(pipefd[1], F_GETFD);
    if (write_flags < 0)
        env->raise_errno();
    if (::fcntl(pipefd[1], F_SETFD, write_flags | O_CLOEXEC | O_NONBLOCK) < 0)
        env->raise_errno();
#else
    if (pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) < 0)
        env->raise_errno();
#endif

    auto io_read = _new(env, klass, { IntegerObject::create(pipefd[0]) }, nullptr);
    auto io_write = _new(env, klass, { IntegerObject::create(pipefd[1]) }, nullptr);
    io_read->as_io()->set_encoding(env, external_encoding, internal_encoding);
    auto pipes = new ArrayObject { io_read, io_write };

    if (!block)
        return pipes;

    Defer close_pipes([&]() {
        io_read->public_send(env, "close"_s);
        io_write->public_send(env, "close"_s);
    });
    return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { pipes }, nullptr);
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
// NATFIXME: Add arguments when those features are
//  added to IOObject::gets()
Value IoObject::readline(Env *env, Value chomp) {
    auto result = gets(env, chomp);
    if (result->is_nil())
        env->raise("EOFError", "end of file reached");
    return result;
}

}
