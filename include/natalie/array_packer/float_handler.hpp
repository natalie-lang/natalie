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
            case 'F':
            case 'f':
                pack_f();
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

        void pack_f() {
            auto value = (float)m_source->to_double();
            m_packed.append((char *)&value, sizeof(float));
        }

        FloatObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
