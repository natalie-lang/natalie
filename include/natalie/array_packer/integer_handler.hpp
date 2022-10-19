#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/integer_object.hpp"
#include "tm/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class IntegerHandler : public Cell {
    public:
        IntegerHandler(IntegerObject *source, Token token)
            : m_source { source }
            , m_token { token } { }

        String pack(Env *env) {
            char d = m_token.directive;
            switch (d) {
            case 'U':
                if (m_source->is_bignum() || m_source->to_nat_int_t() > 0xffffffff || m_source->to_nat_int_t() < 0)
                    env->raise("RangeError", "pack(U): value out of range");
                pack_U();
                break;
            case 'C':
            case 'c':
                pack_c();
                break;
            case 'I':
                pack_I();
                break;
            case 'i':
                pack_i();
                break;
            case 'J':
                pack_J();
                break;
            case 'j':
                pack_j();
                break;
            case 'S':
                pack_S();
                break;
            case 's':
                pack_s();
                break;
            default: {
                char buf[2] = { d, '\0' };
                env->raise("ArgumentError", "unknown directive in string: {}", buf);
            }
            }

            return m_packed;
        }

        virtual void visit_children(Visitor &visitor) override {
            visitor.visit(m_source);
        }

    private:
        void pack_U() {
            auto source = m_source->to_nat_int_t();
            if (source < 128) { // U+007F	    -> 1-byte last code-point
                m_packed.append_char(static_cast<unsigned char>(source));
                return;
            }
            if (source < 2048) { // U+07FF	-> 2-bytes last code-point
                m_packed.append_char(static_cast<unsigned char>(0b11000000 | (source >> 6 & 0b00011111)));
                m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source & 0b00111111)));
                return;
            }
            if (source < 65536) { // U+FFFF	-> 3-bytes last code-point
                m_packed.append_char(static_cast<unsigned char>(0b11100000 | (source >> 12 & 0b00001111)));
                m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source >> 6 & 0b00111111)));
                m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source & 0b00111111)));
                return;
            }
            m_packed.append_char(static_cast<unsigned char>(0b11110000 | (source >> 18 & 0b00000111)));
            m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source >> 12 & 0b00111111)));
            m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source >> 6 & 0b00111111)));
            m_packed.append_char(static_cast<unsigned char>(0b10000000 | (source & 0b00111111)));
        }

        void pack_c() {
            auto source = m_source->to_nat_int_t();
            if (m_source->is_bignum())
                source = (m_source->integer().to_bigint() % 256).to_long_long();

            m_packed.append_char(static_cast<signed char>(source));
        }

        void pack_I() {
            if (m_source->is_bignum()) NAT_NOT_YET_IMPLEMENTED("bignum in Array#pack");
            auto source = (unsigned int)m_source->to_nat_int_t();
            m_packed.append((const char *)(&source), sizeof(source));
        }

        void pack_i() {
            if (m_source->is_bignum()) NAT_NOT_YET_IMPLEMENTED("bignum in Array#pack");
            auto source = (signed int)m_source->to_nat_int_t();
            m_packed.append((const char *)(&source), sizeof(source));
        }

        void pack_J() {
            if (m_source->is_bignum()) {
                pack_j_bignum(sizeof(uintptr_t) * 8);
            } else {
                auto source = (uintptr_t)m_source->to_nat_int_t();
                append_bytes((const char *)(&source), sizeof(source));
            }
        }

        void pack_j() {
            if (m_source->is_bignum()) {
                pack_j_bignum(sizeof(intptr_t) * 8);
            } else {
                auto source = (intptr_t)m_source->to_nat_int_t();
                append_bytes((const char *)(&source), sizeof(source));
            }
        }

        void pack_j_bignum(size_t max_bits) {
            auto digits = m_source->to_bigint().to_binary();

            // TODO: support big endian systems
            assert(system_is_little_endian());

            switch (m_token.endianness) {
            case Endianness::Big: {
                size_t start = std::max((size_t)0, digits.size() - max_bits);
                for (size_t i = start; i < digits.size(); i += 8)
                    append_8_ascii_bits_as_a_byte(digits, i);
                break;
            }
            case Endianness::Little:
            case Endianness::Native:
                for (ssize_t i = max_bits - 7; i >= 0; i -= 8)
                    append_8_ascii_bits_as_a_byte(digits, i);
                break;
            }
        }

        void pack_S() {
            if (m_source->is_bignum()) NAT_NOT_YET_IMPLEMENTED("bignum in Array#pack");
            auto source = (unsigned short)m_source->to_nat_int_t();
            auto size = m_token.native_size ? sizeof(unsigned short) : 2;
            append_bytes((const char *)&source, size);
        }

        void pack_s() {
            if (m_source->is_bignum()) NAT_NOT_YET_IMPLEMENTED("bignum in Array#pack");
            auto source = (signed short)m_source->to_nat_int_t();
            auto size = m_token.native_size ? sizeof(signed short) : 2;
            append_bytes((const char *)&source, size);
        }

        void append_bytes(const char *bytes, int size) {
            switch (m_token.endianness) {
            case Endianness::Native:
                m_packed.append(bytes, size);
                break;
            case Endianness::Big:
                if (system_is_little_endian())
                    for (int i = size - 1; i >= 0; i--)
                        m_packed.append_char(bytes[i]);
                else
                    m_packed.append(bytes, size);
                break;
            case Endianness::Little:
                if (system_is_little_endian())
                    m_packed.append(bytes, size);
                else
                    for (int i = size - 1; i >= 0; i--)
                        m_packed.append_char(bytes[i]);
                break;
            }
        }

        void append_8_ascii_bits_as_a_byte(String &digits, size_t offset) {
            auto len = std::min((size_t)8, digits.size() - offset);
            auto byte = digits.substring(offset, len);
            unsigned char c = 0;
            for (size_t i = 0; i < 8; i++) {
                c *= 2;
                c += byte[i] - '0';
            }
            m_packed.append_char(c);
        };

        bool system_is_little_endian() {
            int i = 1;
            return *((char *)&i) == 1;
        }

        IntegerObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
