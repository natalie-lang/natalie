#include "natalie/array_packer/integer_handler.hpp"
#include "natalie/integer_object.hpp"

namespace Natalie {

namespace ArrayPacker {

    namespace {

        bool system_is_little_endian() {
            int i = 1;
            return *((char *)&i) == 1;
        }

    }

    String IntegerHandler::pack(Env *env) {
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
        case 'L':
            pack_L();
            break;
        case 'l':
            pack_l();
            break;
        case 'N':
            pack_N();
            break;
        case 'n':
            pack_n();
            break;
        case 'Q':
            pack_Q();
            break;
        case 'q':
            pack_q();
            break;
        case 'S':
            pack_S();
            break;
        case 's':
            pack_s();
            break;
        case 'v':
            pack_v();
            break;
        case 'V':
            pack_V();
            break;
        case 'w':
            pack_w(env);
            break;
        default: {
            char buf[2] = { d, '\0' };
            env->raise("ArgumentError", "unknown directive in string: {}", buf);
        }
        }

        return m_packed;
    }

    void IntegerHandler::pack_U() {
        nat_int_t source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % 256).to_long_long();
        else
            source = m_source->to_nat_int_t();

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

    void IntegerHandler::pack_c() {
        nat_int_t source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % 256).to_long_long();
        else
            source = m_source->to_nat_int_t();

        m_packed.append_char(static_cast<signed char>(source));
    }

    void IntegerHandler::pack_I() {
        unsigned int source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % BigInt(::pow(2, 8 * sizeof(unsigned int)))).to_long_long();
        else
            source = (unsigned int)m_source->to_nat_int_t();

        append_bytes((const char *)(&source), sizeof(source));
    }

    void IntegerHandler::pack_i() {
        signed int source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % BigInt(::pow(2, 8 * sizeof(signed int)))).to_long_long();
        else
            source = (signed int)m_source->to_nat_int_t();

        append_bytes((const char *)(&source), sizeof(source));
    }

    void IntegerHandler::pack_J() {
        if (m_source->is_bignum()) {
            pack_bignum(sizeof(uintptr_t) * 8);
        } else {
            auto source = (uintptr_t)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), sizeof(source));
        }
    }

    void IntegerHandler::pack_j() {
        if (m_source->is_bignum()) {
            pack_bignum(sizeof(intptr_t) * 8);
        } else {
            auto source = (intptr_t)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), sizeof(source));
        }
    }

    void IntegerHandler::pack_L() {
        auto size = m_token.native_size ? sizeof(unsigned long) : sizeof(unsigned int);
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (unsigned long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_l() {
        auto size = m_token.native_size ? sizeof(long) : sizeof(int);
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_N() {
        m_token.endianness = Endianness::Big;
        auto size = 4;
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (unsigned long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_n() {
        m_token.endianness = Endianness::Big;
        auto size = 2;
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_Q() {
        auto size = sizeof(uint64_t);
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (uint64_t)m_source->to_nat_int_t();
            append_bytes((const char *)&source, size);
        }
    }

    void IntegerHandler::pack_q() {
        auto size = sizeof(int64_t);
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (int64_t)m_source->to_nat_int_t();
            append_bytes((const char *)&source, size);
        }
    }

    void IntegerHandler::pack_S() {
        unsigned short source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % BigInt(::pow(2, 8 * sizeof(signed int)))).to_long_long();
        else
            source = (unsigned short)m_source->to_nat_int_t();

        auto size = m_token.native_size ? sizeof(unsigned short) : 2;
        append_bytes((const char *)&source, size);
    }

    void IntegerHandler::pack_s() {
        signed short source;
        if (m_source->is_bignum())
            source = (m_source->integer().to_bigint() % BigInt(::pow(2, 8 * sizeof(signed int)))).to_long_long();
        else
            source = (signed short)m_source->to_nat_int_t();

        auto size = m_token.native_size ? sizeof(signed short) : 2;
        append_bytes((const char *)&source, size);
    }

    void IntegerHandler::pack_V() {
        m_token.endianness = Endianness::Little;
        auto size = 4;
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (unsigned long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_v() {
        m_token.endianness = Endianness::Little;
        auto size = 2;
        if (m_source->is_bignum()) {
            pack_bignum(size * 8);
        } else {
            auto source = (long long)m_source->to_nat_int_t();
            append_bytes((const char *)(&source), size);
        }
    }

    void IntegerHandler::pack_w(Env *env) {
        if (m_source->is_negative()) {
            env->raise("ArgumentError", "can't compress negative numbers");
        }

        TM::Vector<char> bytes {};
        size_t size = 0;
        if (m_source->is_bignum()) {
            auto num = m_source->to_bigint();
            do {
                bytes[size] = (num & 0x7f).to_long();
                num = num >> 7;
                if (size > 0) bytes[size] |= 0x80;
                size++;
            } while (num > 0ll);
        } else {
            auto num = m_source->to_nat_int_t();
            do {
                bytes[size] = num & 0x7f;
                num = num >> 7;
                if (size > 0) bytes[size] |= 0x80;
                size++;
            } while (num > 0);
        }
        for (ssize_t i = size - 1; i >= 0; i--) {
            m_packed.append_char(bytes[i]);
        }
    }

    // NOTE: We probably don't need this monster method, but I could not figure out
    // how to pack 'j'/'J' using the modulus trick. ¯\_(ツ)_/¯
    void IntegerHandler::pack_bignum(size_t max_bits) {
        auto digits = m_source->to_bigint().to_binary();

        // TODO: support big endian systems
        assert(system_is_little_endian());

        ssize_t start = std::max((ssize_t)0, (ssize_t)digits.size() - (ssize_t)max_bits);

        switch (m_token.endianness) {
        case Endianness::Big: {
            for (size_t i = start; i < digits.size(); i += 8)
                append_8_ascii_bits_as_a_byte(digits, i);
            break;
        }
        case Endianness::Little:
        case Endianness::Native:
            for (ssize_t i = digits.size() - 8; i >= start; i -= 8)
                append_8_ascii_bits_as_a_byte(digits, i);
            break;
        }
    }

    void IntegerHandler::append_bytes(const char *bytes, int size) {
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

    void IntegerHandler::append_8_ascii_bits_as_a_byte(String &digits, size_t offset) {
        auto len = std::min((size_t)8, digits.size() - offset);
        auto byte = digits.substring(offset, len);
        unsigned char c = 0;
        for (size_t i = 0; i < 8; i++) {
            c *= 2;
            c += byte[i] - '0';
        }
        m_packed.append_char(c);
    }
}

}
