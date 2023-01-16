#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "tm/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class IntegerHandler : public Cell {
    public:
        IntegerHandler(IntegerObject *source, Token token)
            : m_source { source }
            , m_token { token } { }

        String pack(Env *env);

        virtual void visit_children(Visitor &visitor) override {
            visitor.visit(m_source);
        }

    private:
        void pack_U();
        void pack_c();
        void pack_I();
        void pack_i();
        void pack_J();
        void pack_j();
        void pack_L();
        void pack_l();
        void pack_N();
        void pack_n();
        void pack_Q();
        void pack_q();
        void pack_S();
        void pack_s();
        void pack_V();
        void pack_v();
        void pack_w(Env *env);
        void pack_bignum(size_t max_bits);
        void append_bytes(const char *bytes, int size);
        void append_8_ascii_bits_as_a_byte(String &digits, size_t offset);

        IntegerObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
