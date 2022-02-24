#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "tm/string.hpp"
#include <array>

namespace Natalie {

namespace ArrayPacker {

    class StringHandler {
    public:
        StringHandler(String source, Token token)
            : m_source { source }
            , m_token { token } { }

        String pack(Env *env) {
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
            case 'm':
                pack_m();
                break;
            case 'u':
                pack_u();
                break;
            case 'Z':
                if (m_token.star) {
                    while (!at_end())
                        pack_a();
                    if (m_packed.length() == 0 || m_packed.last_char() != '\x00')
                        m_packed.append_char('\x00');
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
                m_packed.append_char('\x00');
            else
                m_packed.append_char(next());
        }

        void pack_A() {
            if (at_end())
                m_packed.append_char(' ');
            else
                m_packed.append_char(next());
        }

        void pack_b() {
            if (at_end())
                return;
            size_t bit_index = m_index % 8;
            unsigned char d = next();
            unsigned char c = bit_index > 0 ? m_packed.pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = bit_index++;
            m_packed.append_char((unsigned char)(c | (b << shift_amount)));
        }

        void pack_B() {
            if (at_end())
                return;
            size_t bit_index = m_index % 8;
            unsigned char d = next();
            unsigned char c = bit_index > 0 ? m_packed.pop_char() : 0;
            unsigned char b = 0;
            if (d == '0' || d == '1')
                b = d - 48;
            else
                b = d & 1;
            auto shift_amount = 7 - bit_index++;
            m_packed.append_char((unsigned char)(c | (b << shift_amount)));
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
                m_packed.append_char(hex_char_to_nibble(c));
            } else {
                m_packed.append_char((hex_char_to_nibble(c) << 4) | m_packed.pop_char());
            }
        }

        void pack_H() {
            bool is_first_nibble = m_index % 2 == 0;
            unsigned char c = next();

            if (is_first_nibble) {
                m_packed.append_char(hex_char_to_nibble(c) << 4);
            } else {
                m_packed.append_char(hex_char_to_nibble(c) | m_packed.pop_char());
            }
        }

        unsigned char ascii_to_uu(unsigned char ascii) {
            constexpr unsigned char six_bit_mask = 0b00111111;
            unsigned char six_bit = ascii & six_bit_mask;
            return six_bit == 0 ? '`' : (six_bit + 32);
        }

        std::array<char, 4> encode_triplet(char a, char b, char c) {
            static constexpr char encode_table[] = {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                '3', '4', '5', '6', '7', '8', '9', '+', '/'
            };

            auto const bits = (a << 16) | (b << 8) | c;

            auto const b64_first = encode_table[(bits >> 18) & 0b0011'1111];
            auto const b64_second = encode_table[(bits >> 12) & 0b0011'1111];
            auto const b64_third = encode_table[(bits >> 6) & 0b0011'1111];
            auto const b64_fourth = encode_table[bits & 0b0011'1111];

            return { b64_first, b64_second, b64_third, b64_fourth };
        }

        void pack_m() {
            bool has_valid_count = !m_token.star && m_token.count >= 0;
            size_t count = has_valid_count ? m_token.count : 60;

            auto size = (int)m_source.size();
            auto triples = size / 3;
            auto push = [&](char b64) {
                m_packed.append_char(b64);

                if (m_packed.size() == count)
                    m_packed.append_char('\n');
            };

            for (auto i = 0; i < triples; ++i) {
                auto first = next();
                auto second = next();
                auto third = next();

                auto base64_chars = encode_triplet(first, second, third);

                push(base64_chars[0]);
                push(base64_chars[1]);
                push(base64_chars[2]);
                push(base64_chars[3]);
            }

            auto const remaining = size - (triples * 3);
            if (! at_end() && remaining == 2) {
                auto base64_chars = encode_triplet(next(), next(), 0x00);

                push(base64_chars[0]);
                push(base64_chars[1]);
                push(base64_chars[2]);
                push('=');
            } else {
                auto base64_chars = encode_triplet(next(), 0x00, 0x00);

                push(base64_chars[0]);
                push(base64_chars[1]);
                push('=');
                push('=');
            }

            if (count > 0)
                m_packed.append_char('\n');
        }

        void pack_u() {
            bool has_valid_count = !m_token.star && m_token.count > 2;
            size_t count = has_valid_count ? (m_token.count - (m_token.count % 3)) : 45;

            while (!at_end()) {
                auto starting_index = m_index;
                auto start_of_packed_string = m_packed.length();
                auto compute_number_of_bytes = [&]() {
                    return std::min(m_index, m_source.length()) - starting_index;
                };
                while (!at_end() && compute_number_of_bytes() < count) {
                    unsigned char first = next();
                    unsigned char second = next();
                    unsigned char third = next();

                    m_packed.append_char(ascii_to_uu(first >> 2));
                    m_packed.append_char(ascii_to_uu(first << 4 | second >> 4));
                    m_packed.append_char(ascii_to_uu(second << 2 | third >> 6));
                    m_packed.append_char(ascii_to_uu(third));
                }
                m_packed.insert(start_of_packed_string, ascii_to_uu(has_valid_count ? compute_number_of_bytes() : std::min(count, compute_number_of_bytes())));
                m_packed.append_char('\n');
            }
        }

        bool at_end() { return m_index >= m_source.length(); }

        unsigned char next() {
            auto is_end = at_end();
            auto i = m_index++;
            if (is_end)
                return 0;
            return m_source.at(i);
        }

        String m_source {};
        Token m_token;
        String m_packed {};
        size_t m_index { 0 };
    };

}

}
