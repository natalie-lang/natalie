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
            if (m_source->is_bignum()) {
                source = (m_source->integer().to_bigint() % 256).to_long_long();
            }

            m_packed.append_char(static_cast<signed char>(source));
        }

        void pack_I() {
            auto source = (unsigned int)m_source->to_nat_int_t();
            m_packed.append(String((const char *)(&source), sizeof(source)));
        }

        void pack_i() {
            auto source = (signed int)m_source->to_nat_int_t();
            m_packed.append(String((const char *)(&source), sizeof(source)));
        }

        IntegerObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
