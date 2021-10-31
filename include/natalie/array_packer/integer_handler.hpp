#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class IntegerHandler {
    public:
        IntegerHandler(nat_int_t source, Token token)
            : m_source { source }
            , m_token { token }
            , m_packed { new String } { }

        String *pack(Env *env) {
            signed char d = m_token.directive;
            switch (d) {
            case 'U':
                pack_U();
                break;
            default: {
                char buf[2] = { d, '\0' };
                env->raise("ArgumentError", "unknown directive in string: {}", buf);
            }
            }

            return m_packed;
        }

    private:

        void pack_U() {
            if (m_source < 128) { // U+007F	    -> 1-byte last code-point
                m_packed->append_char(static_cast<unsigned char>(m_source));
                return;
            }
            if (m_source < 2048) { // U+07FF	-> 2-bytes last code-point
                m_packed->append_char(static_cast<unsigned char>(0b11000000 | (m_source >> 6 & 0b00011111)));
                m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source & 0b00111111)));
                return;
            }
            if (m_source < 65536) { // U+FFFF	-> 3-bytes last code-point
                m_packed->append_char(static_cast<unsigned char>(0b11100000 | (m_source >> 12 & 0b00001111)));
                m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source >> 6 & 0b00111111)));
                m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source & 0b00111111)));
                return;
            }
            m_packed->append_char(static_cast<unsigned char>(0b11110000 | (m_source >> 18 & 0b00000111)));
            m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source >> 12 & 0b00111111)));
            m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source >> 6 & 0b00111111)));
            m_packed->append_char(static_cast<unsigned char>(0b10000000 | (m_source & 0b00111111)));
        }

        nat_int_t m_source;
        Token m_token;
        String *m_packed;
    };

}

}
