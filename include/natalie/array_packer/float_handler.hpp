#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "tm/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class FloatHandler : public Cell {
    public:
        FloatHandler(FloatObject *source, Token token)
            : m_source { source }
            , m_token { token } { }

        String pack(Env *env);

        virtual void visit_children(Visitor &visitor) override {
            visitor.visit(m_source);
        }

    private:
        void pack_d();
        void pack_d_reverse();
        void pack_E();
        void pack_e();
        void pack_f();
        void pack_f_reverse();
        void pack_G();
        void pack_g();

        FloatObject *m_source;
        Token m_token;
        String m_packed {};
    };

}

}
