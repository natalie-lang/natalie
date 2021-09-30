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
                if (m_token.star) {
                    while (!at_end())
                        pack_a();
                } else if (m_token.count != -1) {
                    for (int i = 0; i < m_token.count; ++i)
                        pack_a();
                } else {
                    pack_a();
                }
                break;
            case 'A':
                if (m_token.star) {
                    while (!at_end())
                        pack_A();
                } else if (m_token.count != -1) {
                    for (int i = 0; i < m_token.count; ++i)
                        pack_A();
                } else {
                    pack_A();
                }
                break;
            case 'b':
                if (m_token.star) {
                    while (!at_end())
                        pack_b();
                } else if (m_token.count != -1) {
                    for (int i = 0; i < m_token.count; ++i)
                        pack_b();
                } else {
                    pack_b();
                }
                break;
            case 'B':
                if (m_token.star) {
                    while (!at_end())
                        pack_B();
                } else if (m_token.count != -1) {
                    for (int i = 0; i < m_token.count; ++i)
                        pack_B();
                } else {
                    pack_B();
                }
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

    private:
        void pack_a() {
            if (at_end())
                m_packed->append_char('\x00');
            else
                m_packed->append_char(m_source->at(m_index++));
        }

        void pack_A() {
            if (at_end())
                m_packed->append_char(' ');
            else
                m_packed->append_char(m_source->at(m_index++));
        }

        void pack_b() {
            if (at_end())
                return;
            unsigned char d = m_source->at(m_index++);
            unsigned char c = m_bit_index > 0 ? m_packed->pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = m_bit_index++;
            m_packed->append_char((unsigned char)(c | (b << shift_amount)));
            if (m_bit_index >= 8)
                m_bit_index = 0;
        }

        void pack_B() {
            if (at_end())
                return;
            unsigned char d = m_source->at(m_index++);
            unsigned char c = m_bit_index > 0 ? m_packed->pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = 7 - m_bit_index++;
            m_packed->append_char((unsigned char)(c | (b << shift_amount)));
            if (m_bit_index >= 8)
                m_bit_index = 0;
        }

        bool at_end() { return m_index >= m_source->length(); }

        String *m_source;
        Token m_token;
        String *m_packed;
        size_t m_index { 0 };
        size_t m_bit_index { 0 };
    };

}

}
