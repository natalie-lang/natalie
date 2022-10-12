#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/integer_object.hpp"
#include "tm/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class FloatHandler : public Cell {
    public:
        FloatHandler(FloatObject *source, Token token)
            : m_source { source }
            , m_token { token } { }

        String pack(Env *env) {
            char d = m_token.directive;
            switch (d) {
            case 'D':
            case 'd':
                pack_d();
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
        void pack_d() {
            /*
             * significand bits = 53
             * decimal digits   = 15.95
             * exponent bits    = 11
             * exponent bias    = (2^10)−1 = 1023
             */
            auto value = m_source->to_double();
            auto double_bytes = (unsigned char *)&value;
            for (size_t i = 0; i < sizeof(double); i++) {
                m_packed.append_char((unsigned char)double_bytes[i]);
            }
        }

        FloatObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
