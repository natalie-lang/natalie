#include "natalie/array_packer/integer_handler.hpp"
#include "natalie/integer_methods.hpp"
#include <limits>

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
            if (m_source.is_bignum() || m_source > static_cast<long long>(0xffffffff) || m_source < 0)
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
        if (m_source.is_bignum())
            source = (m_source % 256).to_nat_int_t();
        else
            source = m_source.to_nat_int_t();

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
        auto source = m_source.to_nat_int_t_or_none();
        if (!source)
            source = (m_source % 256).to_nat_int_t();
        m_packed.append_char(static_cast<signed char>(source.value()));
    }

    void IntegerHandler::pack_I() {
        auto source = m_source.to_nat_int_t_or_none();
        unsigned int source_cast;
        if (source)
            source_cast = (unsigned int)source.value();
        else
            source_cast = (m_source % BigInt(::pow(2, std::numeric_limits<unsigned int>::digits))).to_nat_int_t();

        append_bytes((const char *)(&source_cast), sizeof(source_cast));
    }

    void IntegerHandler::pack_i() {
        auto source = m_source.to_nat_int_t_or_none();
        signed int source_cast;
        if (source)
            source_cast = (signed int)source.value();
        else
            source_cast = (m_source % BigInt(::pow(2, std::numeric_limits<signed int>::digits))).to_nat_int_t();

        append_bytes((const char *)(&source_cast), sizeof(source_cast));
    }

    void IntegerHandler::pack_J() {
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (uintptr_t)source.value();
            append_bytes((const char *)(&source_cast), sizeof(source_cast));
        } else {
            pack_bignum(sizeof(uintptr_t) * 8);
        }
    }

    void IntegerHandler::pack_j() {
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (intptr_t)source.value();
            append_bytes((const char *)(&source_cast), sizeof(source_cast));
        } else {
            pack_bignum(sizeof(intptr_t) * 8);
        }
    }

    void IntegerHandler::pack_L() {
        auto size = m_token.native_size ? sizeof(unsigned long) : sizeof(unsigned int);
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (unsigned long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_l() {
        auto size = m_token.native_size ? sizeof(long) : sizeof(int);
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_N() {
        m_token.endianness = Endianness::Big;
        auto source = m_source.to_nat_int_t_or_none();
        auto size = 4;
        if (source) {
            auto source_cast = (unsigned long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_n() {
        m_token.endianness = Endianness::Big;
        auto source = m_source.to_nat_int_t_or_none();
        auto size = 2;
        if (source) {
            auto source_cast = (long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_Q() {
        auto size = sizeof(uint64_t);
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (uint64_t)source.value();
            append_bytes((const char *)&source_cast, size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_q() {
        auto size = sizeof(int64_t);
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto source_cast = (int64_t)source.value();
            append_bytes((const char *)&source_cast, size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_S() {
        auto source = m_source.to_nat_int_t_or_none();
        unsigned short source_cast;
        if (source)
            source_cast = (unsigned short)source.value();
        else
            source_cast = (m_source % BigInt(::pow(2, std::numeric_limits<signed int>::digits))).to_nat_int_t();

        auto size = m_token.native_size ? sizeof(unsigned short) : 2;
        append_bytes((const char *)&source_cast, size);
    }

    void IntegerHandler::pack_s() {
        auto source = m_source.to_nat_int_t_or_none();
        signed short source_cast;
        if (source)
            source_cast = (signed short)source.value();
        else
            source_cast = (m_source % BigInt(::pow(2, std::numeric_limits<signed int>::digits))).to_nat_int_t();

        auto size = m_token.native_size ? sizeof(signed short) : 2;
        append_bytes((const char *)&source_cast, size);
    }

    void IntegerHandler::pack_V() {
        m_token.endianness = Endianness::Little;
        auto source = m_source.to_nat_int_t_or_none();
        auto size = 4;
        if (source) {
            auto source_cast = (unsigned long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_v() {
        m_token.endianness = Endianness::Little;
        auto source = m_source.to_nat_int_t_or_none();
        auto size = 2;
        if (source) {
            auto source_cast = (long long)source.value();
            append_bytes((const char *)(&source_cast), size);
        } else {
            pack_bignum(size * 8);
        }
    }

    void IntegerHandler::pack_w(Env *env) {
        if (m_source.is_negative())
            env->raise("ArgumentError", "can't compress negative numbers");

        TM::Vector<char> bytes {};
        size_t size = 0;
        auto source = m_source.to_nat_int_t_or_none();
        if (source) {
            auto num = source.value();
            do {
                bytes[size] = num & 0x7f;
                num = num >> 7;
                if (size > 0) bytes[size] |= 0x80;
                size++;
            } while (num > 0);
        } else {
            auto num = m_source.to_bigint();
            do {
                bytes[size] = (num & 0x7f).to_long();
                num = num >> 7;
                if (size > 0) bytes[size] |= 0x80;
                size++;
            } while (num > 0ll);
        }
        for (ssize_t i = size - 1; i >= 0; i--) {
            m_packed.append_char(bytes[i]);
        }
    }

    // NOTE: We probably don't need this monster method, but I could not figure out
    // how to pack 'j'/'J' using the modulus trick. ¯\_(ツ)_/¯
    void IntegerHandler::pack_bignum(size_t max_bits) {
        auto digits = m_source.to_bigint().to_binary();

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
