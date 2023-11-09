#include "natalie/ioutil.hpp"
#include "natalie.hpp"

namespace Natalie {

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
            path = path->to_str(env);
        path->assert_type(env, Object::Type::String, "String");
        return path->as_string();
    }

    // accepts io or io-like object for fstat
    // accepts path or string like object for stat
    int object_stat(Env *env, Value io, struct stat *sb) {
        if (io->is_io() || io->respond_to(env, "to_io"_s)) {
            auto file_desc = io->to_io(env)->fileno();
            return ::fstat(file_desc, sb);
        }

        io = convert_using_to_path(env, io);
        return ::stat(io->as_string()->c_str(), sb);
    }

    void flags_struct::parse_flags_obj(Env *env, Value flags_obj) {
        if (!flags_obj || flags_obj->is_nil())
            return;

        m_has_mode = true;

        if (!flags_obj->is_integer() && !flags_obj->is_string()) {
            if (flags_obj->respond_to(env, "to_str"_s)) {
                flags_obj = flags_obj->to_str(env);
            } else if (flags_obj->respond_to(env, "to_int"_s)) {
                flags_obj = flags_obj->to_int(env);
            }
        }

        switch (flags_obj->type()) {
        case Object::Type::Integer:
            m_flags = flags_obj->as_integer()->to_nat_int_t();
            break;
        case Object::Type::String: {
            auto colon = new StringObject { ":" };
            auto flagsplit = flags_obj->as_string()->split(env, colon, nullptr)->as_array();
            auto flags_str = flagsplit->fetch(env, IntegerObject::create(static_cast<nat_int_t>(0)), new StringObject { "" }, nullptr)->as_string()->string();
            auto extenc = flagsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(1)), nullptr);
            auto intenc = flagsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(2)), nullptr);
            if (!extenc->is_nil()) m_external_encoding = EncodingObject::find_encoding(env, extenc);
            if (!intenc->is_nil()) m_internal_encoding = EncodingObject::find_encoding(env, intenc);

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

            if (binary_text_mode == 'b') {
                m_read_mode = flags_struct::read_mode::binary;
            } else if (binary_text_mode == 't') {
                m_read_mode = flags_struct::read_mode::text;
            }

            if (main_mode == 'r' && !read_write_mode)
                m_flags = O_RDONLY;
            else if (main_mode == 'r' && read_write_mode == '+')
                m_flags = O_RDWR;
            else if (main_mode == 'w' && !read_write_mode)
                m_flags = O_WRONLY | O_CREAT | O_TRUNC;
            else if (main_mode == 'w' && read_write_mode == '+')
                m_flags = O_RDWR | O_CREAT | O_TRUNC;
            else if (main_mode == 'a' && !read_write_mode)
                m_flags = O_WRONLY | O_CREAT | O_APPEND;
            else if (main_mode == 'a' && read_write_mode == '+')
                m_flags = O_RDWR | O_CREAT | O_APPEND;
            else
                env->raise("ArgumentError", "invalid access mode {}", flags_str);
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of {} into String", flags_obj->klass()->inspect_str());
        }
    }

    void flags_struct::parse_mode(Env *env) {
        if (!m_kwargs) return;
        auto mode = m_kwargs->remove(env, "mode"_s);
        if (!mode || mode->is_nil()) return;
        if (has_mode())
            env->raise("ArgumentError", "mode specified twice");
        parse_flags_obj(env, mode);
    }

    void flags_struct::parse_flags(Env *env) {
        if (!m_kwargs) return;
        auto flags = m_kwargs->remove(env, "flags"_s);
        if (!flags || flags->is_nil()) return;
        m_flags |= static_cast<int>(flags->to_int(env)->to_nat_int_t());
    }

    void flags_struct::parse_encoding(Env *env) {
        if (!m_kwargs) return;
        auto encoding = m_kwargs->remove(env, "encoding"_s);
        if (!encoding || encoding->is_nil()) return;
        if (m_external_encoding) {
            env->raise("ArgumentError", "encoding specified twice");
        } else if (m_kwargs->has_key(env, "external_encoding"_s)) {
            env->warn("Ignoring encoding parameter '{}', external_encoding is used", encoding);
        } else if (m_kwargs->has_key(env, "internal_encoding"_s)) {
            env->warn("Ignoring encoding parameter '{}', internal_encoding is used", encoding);
        } else if (encoding->is_encoding()) {
            m_external_encoding = encoding->as_encoding();
        } else {
            encoding = encoding->to_str(env);
            if (encoding->as_string()->include(":")) {
                auto colon = new StringObject { ":" };
                auto encsplit = encoding->to_str(env)->split(env, colon, nullptr)->as_array();
                encoding = encsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(0)), nullptr);
                auto internal_encoding = encsplit->ref(env, IntegerObject::create(static_cast<nat_int_t>(1)), nullptr);
                m_internal_encoding = EncodingObject::find_encoding(env, internal_encoding);
            }
            m_external_encoding = EncodingObject::find_encoding(env, encoding);
        }
    }

    void flags_struct::parse_external_encoding(Env *env) {
        if (!m_kwargs) return;
        auto external_encoding = m_kwargs->remove(env, "external_encoding"_s);
        if (!external_encoding || external_encoding->is_nil()) return;
        if (m_external_encoding)
            env->raise("ArgumentError", "encoding specified twice");
        if (external_encoding->is_encoding()) {
            m_external_encoding = external_encoding->as_encoding();
        } else {
            m_external_encoding = EncodingObject::find_encoding(env, external_encoding->to_str(env));
        }
    }

    void flags_struct::parse_internal_encoding(Env *env) {
        if (!m_kwargs) return;
        auto internal_encoding = m_kwargs->remove(env, "internal_encoding"_s);
        if (!internal_encoding || internal_encoding->is_nil()) return;
        if (m_internal_encoding)
            env->raise("ArgumentError", "encoding specified twice");
        if (internal_encoding->is_encoding()) {
            m_internal_encoding = internal_encoding->as_encoding();
        } else {
            internal_encoding = internal_encoding->to_str(env);
            if (internal_encoding->as_string()->string() != "-") {
                m_internal_encoding = EncodingObject::find_encoding(env, internal_encoding);
                if (m_external_encoding == m_internal_encoding)
                    m_internal_encoding = nullptr;
            }
        }
    }

    void flags_struct::parse_textmode(Env *env) {
        if (!m_kwargs) return;
        auto textmode = m_kwargs->remove(env, "textmode"_s);
        if (!textmode || textmode->is_nil()) return;
        if (binmode()) {
            env->raise("ArgumentError", "both textmode and binmode specified");
        } else if (this->textmode()) {
            env->raise("ArgumentError", "textmode specified twice");
        }
        if (textmode->is_truthy())
            m_read_mode = flags_struct::read_mode::text;
    }

    void flags_struct::parse_binmode(Env *env) {
        if (!m_kwargs) return;
        auto binmode = m_kwargs->remove(env, "binmode"_s);
        if (!binmode || binmode->is_nil()) return;
        if (this->binmode()) {
            env->raise("ArgumentError", "binmode specified twice");
        } else if (textmode()) {
            env->raise("ArgumentError", "both textmode and binmode specified");
        }
        if (binmode->is_truthy())
            m_read_mode = flags_struct::read_mode::binary;
    }

    void flags_struct::parse_autoclose(Env *env) {
        if (!m_kwargs) return;
        auto autoclose = m_kwargs->remove(env, "autoclose"_s);
        if (!autoclose) return;
        m_autoclose = autoclose->is_truthy();
    }

    void flags_struct::parse_path(Env *env) {
        if (!m_kwargs) return;
        auto path = m_kwargs->remove(env, "path"_s);
        if (!path) return;
        m_path = convert_using_to_path(env, path);
    }

    flags_struct::flags_struct(Env *env, Value flags_obj, HashObject *kwargs)
        : m_kwargs(kwargs) {
        parse_flags_obj(env, flags_obj);
        parse_mode(env);
        parse_flags(env);
        m_flags |= O_CLOEXEC;
        parse_encoding(env);
        parse_external_encoding(env);
        parse_internal_encoding(env);
        parse_textmode(env);
        parse_binmode(env);
        parse_autoclose(env);
        parse_path(env);
        if (!m_external_encoding) {
            if (binmode()) {
                m_external_encoding = EncodingObject::get(Encoding::ASCII_8BIT);
            } else if (textmode()) {
                m_external_encoding = EncodingObject::get(Encoding::UTF_8);
            }
        }
        env->ensure_no_extra_keywords(m_kwargs);
    }

    mode_t perm_to_mode(Env *env, Value perm) {
        if (perm && !perm->is_nil())
            return IntegerObject::convert_to_int(env, perm);
        else
            return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // 0660 default
    }
}

}
