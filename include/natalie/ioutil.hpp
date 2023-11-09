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

        bool has_mode { false };
        int flags { O_RDONLY | O_CLOEXEC };
        read_mode read_mode { read_mode::none };
        EncodingObject *external_encoding { nullptr };
        EncodingObject *internal_encoding { nullptr };
        bool autoclose { false };
        StringObject *path { nullptr };

        // NATFIXME: This should be made private, but we have to shave some yaks first
        HashObject *m_kwargs { nullptr };
    };
    mode_t perm_to_mode(Env *env, Value perm);
}

}
