#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"

namespace Natalie {

class StringUnpacker : public Cell {
    using Tokenizer = ArrayPacker::Tokenizer;
    using Token = ArrayPacker::Token;
    using Endianness = ArrayPacker::Endianness;

public:
    StringUnpacker(const StringObject *source, String directives, nat_int_t offset)
        : m_source { source }
        , m_directives { Tokenizer { directives }.tokenize() }
        , m_index { (size_t)std::max(offset, (nat_int_t)0) } { }

    ~StringUnpacker() { delete m_directives; }

    ArrayObject *unpack(Env *env);
    Value unpack1(Env *env);

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_source);
        visitor.visit(m_unpacked_value);
        visitor.visit(m_unpacked_array);
    }

private:
    void unpack_token(Env *env, Token &);

    void unpack_A(Token &token);
    void unpack_a(Token &token);
    void unpack_B(Token &token);
    void unpack_C(Token &token);
    void unpack_c(Token &token);
    void unpack_b(Token &token);
    void unpack_H(Token &token);
    void unpack_h(Token &token);
    void unpack_M(Env *env, Token &token);
    void unpack_m(Env *env, Token &token);
    void unpack_P(Token &token);
    void unpack_p();
    void unpack_U(Env *env, Token &token);
    void unpack_u(Token &token);
    void unpack_w(Env *env, Token &token);
    void unpack_X(Env *env, Token &token);
    void unpack_x(Env *env, Token &token);
    void unpack_Z(Token &token);
    void unpack_at(Env *env, Token &token);

    template <typename T>
    void unpack_float(Token &token, bool little_endian) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if ((m_index + sizeof(T)) > m_source->length()) {
                if (!token.star)
                    append(NilObject::the());
                m_index++;
            } else {
                // reverse a character buffer based on endianness
                auto out = String();
                for (size_t idx = 0; idx < sizeof(T); idx++) {
                    auto realidx = (little_endian) ? idx : (sizeof(T) - 1 - idx);
                    out.append_char(pointer()[realidx]);
                }
                m_index += sizeof(T);
                double double_val = double(*(T *)out.c_str());
                auto fltobj = new FloatObject { double_val };
                append(fltobj);
            }
            consumed++;
        }
    }

    template <typename T>
    void unpack_int(Token &token) {
        bool little_endian = (token.endianness == ArrayPacker::Endianness::Little) || (token.endianness == Endianness::Native && system_is_little_endian());
        unpack_int<T>(token, little_endian);
    }

    template <typename T>
    void unpack_int(Token &token, bool little_endian) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if ((m_index + sizeof(T)) > m_source->length()) {
                if (!token.star)
                    append(NilObject::the());
                m_index++;
            } else {
                // NATFIXME: this method of fixing endianness may not be efficient
                // reverse a character buffer based on endianness
                auto out = String();
                for (size_t idx = 0; idx < sizeof(T); idx++) {
                    auto realidx = (little_endian) ? idx : (sizeof(T) - 1 - idx);
                    out.append_char(pointer()[realidx]);
                }
                m_index += sizeof(T);
                // Previosly had pushed Value::integer() values, but for large unsigned values
                // it produced incorrect results.
                auto bigint = BigInt(*(T *)out.c_str());
                append(IntegerObject::create(Integer(std::move(bigint))));
            }
            consumed++;
        }
    }

    template <typename Fn>
    size_t unpack_bytes(Token &token, Fn handler) {
        size_t start = m_index;
        if (token.star) {
            while (!at_end()) {
                auto keep_going = handler(next_char());
                if (!keep_going)
                    break;
            }
        } else if (token.count != -1) {
            for (int i = 0; i < token.count; ++i) {
                if (at_end())
                    break;
                auto keep_going = handler(next_char());
                if (!keep_going)
                    break;
            }
        } else if (!at_end()) {
            handler(next_char());
        }
        assert(start <= m_index); // going backwards is unexpected
        return m_index - start;
    }

    template <typename Fn>
    void unpack_nibbles(Token &token, Fn handler) {
        if (token.star) {
            while (!at_end())
                handler(next_char(), 2);
        } else if (token.count != 0) {
            auto count = std::max(1, token.count);
            while (count > 0 && !at_end()) {
                handler(next_char(), count >= 2 ? 2 : 1);
                count -= 2;
            }
        }
    }

    template <typename Fn>
    void unpack_bits(Token &token, Fn handler) {
        auto bits_remaining = token.count;
        if (bits_remaining < 0)
            bits_remaining = 1;
        if (token.star)
            bits_remaining = 8;

        while (bits_remaining > 0 && !at_end()) {
            auto c = next_char();

            for (int i = 0; i < std::min(8, bits_remaining); i++)
                c = handler(c);

            if (token.star)
                bits_remaining = 8;
            else if (bits_remaining >= 8)
                bits_remaining -= 8;
            else
                bits_remaining = 0;
        }
    }

    const char *pointer() const { return m_source->c_str() + m_index; }

    unsigned char next_char() {
        if (at_end())
            return 0;
        return m_source->at(m_index++);
    }

    unsigned char peek_char() const {
        if (at_end())
            return 0;
        return m_source->at(m_index);
    }

    bool at_end() const { return m_index >= m_source->length(); }

    bool system_is_little_endian() const {
        int i = 1;
        return *((char *)&i) == 1;
    }

    void append(Value value) {
        // Avoid building an array for the String#unpack1 case.
        if (!m_unpacked_value) {
            m_unpacked_value = value;
        } else {
            if (!m_unpacked_array) {
                m_unpacked_array = new ArrayObject {};
                m_unpacked_array->push(m_unpacked_value);
            }
            m_unpacked_array->push(value);
        }
    }

    ArrayObject *unpacked_array() {
        if (m_unpacked_array)
            return m_unpacked_array;
        else if (m_unpacked_value)
            return new ArrayObject { m_unpacked_value };
        return new ArrayObject {};
    }

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    Value m_unpacked_value { nullptr };
    ArrayObject *m_unpacked_array { nullptr };
    size_t m_index { 0 };
};
}
