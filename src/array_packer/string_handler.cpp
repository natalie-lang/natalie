#include "natalie/array_packer/string_handler.hpp"
#include <array>

namespace Natalie {

namespace ArrayPacker {

    namespace {

        unsigned char hex_char_to_nibble(unsigned char c) {
            if (c >= '0' && c <= '9')
                c -= '0';
            else if (c >= 'A' && c <= 'Z')
                c -= 'A' - 0xA;
            else if (c >= 'a' && c <= 'z')
                c -= 'a' - 0xA;

            return c & 0x0f;
        }

        unsigned char ascii_to_uu(unsigned char ascii) {
            constexpr unsigned char six_bit_mask = 0b00111111;
            unsigned char six_bit = ascii & six_bit_mask;
            return six_bit == 0 ? '`' : (six_bit + 32);
        }

        std::array<unsigned char, 4> base64_encode_triplet(unsigned char a, unsigned char b, unsigned char c) {
            static constexpr unsigned char encode_table[] = {
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

    }

    String StringHandler::pack(Env *env) {
        char d = m_token.directive;
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
        case 'M':
            pack_M();
            break;
        case 'm':
            pack_m();
            break;
        case 'P':
            pack_P();
            break;
        case 'p':
            pack_p();
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

    void StringHandler::pack_with_loop(PackHandlerFn handler) {
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

    void StringHandler::pack_a() {
        if (at_end())
            m_packed.append_char('\x00');
        else
            m_packed.append_char(next());
    }

    void StringHandler::pack_A() {
        if (at_end())
            m_packed.append_char(' ');
        else
            m_packed.append_char(next());
    }

    void StringHandler::pack_b() {
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

    void StringHandler::pack_B() {
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

    void StringHandler::pack_h() {
        bool is_first_nibble = m_index % 2 == 0;
        unsigned char c = next();

        if (is_first_nibble) {
            m_packed.append_char(hex_char_to_nibble(c));
        } else {
            m_packed.append_char((hex_char_to_nibble(c) << 4) | m_packed.pop_char());
        }
    }

    void StringHandler::pack_H() {
        bool is_first_nibble = m_index % 2 == 0;
        unsigned char c = next();

        if (is_first_nibble) {
            m_packed.append_char(hex_char_to_nibble(c) << 4);
        } else {
            m_packed.append_char(hex_char_to_nibble(c) | m_packed.pop_char());
        }
    }

    void StringHandler::pack_M() {
        size_t count = m_token.count;
        if (m_token.star || m_token.count == -1 || m_token.count == 0 || m_token.count == 1)
            count = 72;

        size_t line_size = 0;

        for (size_t index = 0; index < m_source.size(); index++) {
            unsigned char c = m_source[index];

            if (c == '\t' || c == '\n' || (isprint(c) && c != '=' && (unsigned int)c <= 0176)) {
                m_packed.append_char(c);
                line_size++;
                if (c == '\n')
                    line_size = 0;
            } else {
                m_packed.append_sprintf("=%02X", (unsigned int)c);
                line_size += 3;
            }

            auto at_end_of_line = [&]() {
                return index + 1 >= m_source.size() || m_source[index + 1] == '\n';
            };

            if ((c == '\t' || c == ' ') && at_end_of_line()) {
                m_packed.append("=\n");
                line_size = 0;
            }

            if (count && line_size > count) {
                m_packed.append("=\n");
                line_size = 0;
            }
        }

        if (line_size > 0)
            m_packed.append("=\n");
    }

    void StringHandler::pack_m() {
        if (m_source.is_empty())
            return;

        bool has_valid_count = !m_token.star && (m_token.count == 0 || m_token.count >= 3);
        size_t count = has_valid_count ? m_token.count : 45;
        auto per_line_char_count = (count / 3) * 4;

        auto size = (int)m_source.size();
        auto triples = size / 3;
        size_t chars_pushed = 0;

        auto push = [&](unsigned char b64) {
            chars_pushed++;
            m_packed.append_char(b64);

            if (per_line_char_count != 0 && chars_pushed % per_line_char_count == 0)
                m_packed.append_char('\n');
        };

        for (auto i = 0; i < triples; ++i) {
            auto first = next();
            auto second = next();
            auto third = next();

            auto base64_chars = base64_encode_triplet(first, second, third);

            push(base64_chars[0]);
            push(base64_chars[1]);
            push(base64_chars[2]);
            push(base64_chars[3]);
        }

        if (!at_end()) {
            auto const remaining = size - (triples * 3);
            if (remaining == 2) {
                auto first = next();
                auto second = next();
                auto third = 0x00;
                auto base64_chars = base64_encode_triplet(first, second, third);

                push(base64_chars[0]);
                push(base64_chars[1]);
                push(base64_chars[2]);
                push('=');
            } else if (remaining == 1) {
                auto first = next();
                auto second = 0x00;
                auto third = 0x00;
                auto base64_chars = base64_encode_triplet(first, second, third);

                push(base64_chars[0]);
                push(base64_chars[1]);
                push('=');
                push('=');
            }
        }

        if (count > 0 && !m_packed.is_empty() && m_packed.last_char() != '\n')
            m_packed.append_char('\n');
    }

    void StringHandler::pack_P() {
        if (!m_string_object) {
            for (size_t i = 0; i < sizeof(uintptr_t); i++)
                m_packed.append_char(0x0);
            return;
        }
        auto c_str = m_string_object->c_str();
        auto ptr = (const char *)&c_str;
        for (size_t i = 0; i < sizeof(uintptr_t); i++)
            m_packed.append_char(ptr[i]);
    }

    void StringHandler::pack_p() {
        if (!m_string_object) {
            for (size_t i = 0; i < sizeof(uintptr_t); i++)
                m_packed.append_char(0x0);
            return;
        }
        auto c_str = m_string_object->c_str();
        auto ptr = (const char *)&c_str;
        for (size_t i = 0; i < sizeof(uintptr_t); i++)
            m_packed.append_char(ptr[i]);
    }

    void StringHandler::pack_u() {
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

    unsigned char StringHandler::next() {
        auto is_end = at_end();
        auto i = m_index++;
        if (is_end)
            return 0;
        return m_source.at(i);
    }

}

}
