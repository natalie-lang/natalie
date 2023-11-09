#pragma once

#include "natalie.hpp"

#include <fcntl.h>

namespace Natalie {

namespace ioutil {
    // Utility Functions Common to File, Dir and Io
    StringObject *convert_using_to_path(Env *env, Value path);
    int object_stat(Env *env, Value io, struct stat *sb);
    struct flags_struct {
        enum class read_mode { none,
            text,
            binary };

        flags_struct(Env *env, Value flags_obj, HashObject *kwargs);

        bool has_mode() const { return m_has_mode; }
        int flags() const { return m_flags; }
        EncodingObject *external_encoding() const { return m_external_encoding; }
        EncodingObject *internal_encoding() const { return m_internal_encoding; }
        bool textmode() const { return m_read_mode == read_mode::text; }
        bool binmode() const { return m_read_mode == read_mode::binary; }
        StringObject *path() const { return m_path; }
        bool autoclose() const { return m_autoclose; }

    private:
        void parse_flags_obj(Env *, Value);
        void parse_mode(Env *);
        void parse_flags(Env *);
        void parse_encoding(Env *);
        void parse_external_encoding(Env *);
        void parse_internal_encoding(Env *);
        void parse_textmode(Env *);
        void parse_binmode(Env *);
        void parse_autoclose(Env *);
        void parse_path(Env *);

        HashObject *m_kwargs { nullptr };
        bool m_has_mode { false };
        int m_flags { O_RDONLY | O_CLOEXEC };
        read_mode m_read_mode { read_mode::none };
        EncodingObject *m_external_encoding { nullptr };
        EncodingObject *m_internal_encoding { nullptr };
        bool m_autoclose { false };
        StringObject *m_path { nullptr };
    };
    mode_t perm_to_mode(Env *env, Value perm);
}

}
