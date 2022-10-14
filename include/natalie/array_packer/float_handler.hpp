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
            case 'E':
                pack_E();
                break;
            case 'e':
                pack_e();
                break;
            case 'F':
            case 'f':
                pack_f();
                break;
            case 'G':
                pack_G();
                break;
            case 'g':
                pack_g();
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
            auto value = m_source->to_double();
            m_packed.append((char *)&value, sizeof(double));
        }

        void pack_d_reverse() {
            auto value = m_source->to_double();
            for (int i = sizeof(double) - 1; i >= 0; i--)
                m_packed.append_char(((char *)&value)[i]);
        }

        void pack_E() {
            if (system_is_little_endian())
                pack_d();
            else
                pack_d_reverse();
        }

        void pack_e() {
            if (system_is_little_endian())
                pack_f();
            else
                pack_f_reverse();
        }

        void pack_f() {
            auto value = (float)m_source->to_double();
            m_packed.append((char *)&value, sizeof(float));
        }

        void pack_f_reverse() {
            auto value = (float)m_source->to_double();
            for (int i = sizeof(float) - 1; i >= 0; i--)
                m_packed.append_char(((char *)&value)[i]);
        }

        void pack_G() {
            if (system_is_little_endian())
                pack_d_reverse();
            else
                pack_d();
        }

        void pack_g() {
            if (system_is_little_endian())
                pack_f_reverse();
            else
                pack_f();
        }

        bool system_is_little_endian() {
            int i = 1;
            return *((char *)&i) == 1;
        }

        FloatObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
