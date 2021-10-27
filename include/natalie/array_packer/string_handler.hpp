#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class StringHandler {
    public:
        StringHandler(String *source, Token token)
            : m_source { source }
            , m_token { token }
            , m_packed { new String } { }

        String *pack(Env *env) {
            signed char d = m_token.directive;
            switch (d) {
            case 'a':
                pack_with_loop(&StringHandler::pack_a);
                break;
            case 'A':
                pack_with_loop(&StringHandler::pack_A);
                break;
            case 'b':
                pack_with_loop(&StringHandler::pack_b);
                break;
            case 'B':
                pack_with_loop(&StringHandler::pack_B);
                break;
            case 'h':
                pack_with_loop(&StringHandler::pack_h);
                break;
            case 'H':
                pack_with_loop(&StringHandler::pack_H);
                break;
            case 'Z':
                if (m_token.star) {
                    while (!at_end())
                        pack_a();
                    if (m_packed->length() == 0 || m_packed->last_char() != '\x00')
                        m_packed->append_char('\x00');
                } else if (m_token.count != -1) {
                    for (int i = 0; i < m_token.count; ++i)
                        pack_a();
                } else {
                    pack_a();
                }
                break;
            default: {
                char buf[2] = { d, '\0' };
                env->raise("ArgumentError", "unknown directive in string: {}", buf);
            }
            }

            return m_packed;
        }

        using PackHandlerFn = void (StringHandler::*)();

    private:
        void pack_with_loop(PackHandlerFn handler) {
            if (m_token.star) {
                while (!at_end())
                    (this->*handler)();
            } else if (m_token.count != -1) {
                for (int i = 0; i < m_token.count; ++i)
                    (this->*handler)();
            } else {
                (this->*handler)();
            }
        }

        void pack_a() {
            if (at_end())
                m_packed->append_char('\x00');
            else
                m_packed->append_char(next());
        }

        void pack_A() {
            if (at_end())
                m_packed->append_char(' ');
            else
                m_packed->append_char(next());
        }

        void pack_b() {
            if (at_end())
                return;
            size_t bit_index = m_index % 8;
            unsigned char d = next();
            unsigned char c = bit_index > 0 ? m_packed->pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = bit_index++;
            m_packed->append_char((unsigned char)(c | (b << shift_amount)));
        }

        void pack_B() {
            if (at_end())
                return;
            size_t bit_index = m_index % 8;
            unsigned char d = next();
            unsigned char c = bit_index > 0 ? m_packed->pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = 7 - bit_index++;
            m_packed->append_char((unsigned char)(c | (b << shift_amount)));
        }

        unsigned char hex_char_to_nibble(unsigned char c) {
            if (c >= '0' && c <= '9')
                c -= '0';
            else if (c >= 'A' && c <= 'Z')
                c -= 'A' - 0xA;
            else if (c >= 'a' && c <= 'z')
                c -= 'a' - 0xA;

            return c & 0x0f;
        }

        void pack_h() {
            bool is_first_nibble = m_index % 2 == 0;
            unsigned char c = next();

            if (is_first_nibble) {
                m_packed->append_char(hex_char_to_nibble(c));
            } else {
                m_packed->append_char((hex_char_to_nibble(c) << 4) | m_packed->pop_char());
            }
        }

        void pack_H() {
            bool is_first_nibble = m_index % 2 == 0;
            unsigned char c = next();

            if (is_first_nibble) {
                m_packed->append_char(hex_char_to_nibble(c) << 4);
            } else {
                m_packed->append_char(hex_char_to_nibble(c) | m_packed->pop_char());
            }
        }

        bool at_end() { return m_index >= m_source->length(); }

        unsigned char next() {
            auto is_end = at_end();
            auto i = m_index++;
            if (is_end)
                return 0;
            return m_source->at(i); 
        }

        String *m_source;
        Token m_token;
        String *m_packed;
        size_t m_index { 0 };
    };

}

}
