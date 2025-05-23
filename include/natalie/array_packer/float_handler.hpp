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

        virtual void visit_children(Visitor &visitor) const override {
            visitor.visit(m_source);
        }

        virtual TM::String dbg_inspect(int indent = 0) const override {
            return TM::String::format("<ArrayPacker::FloatHandler {h}>", this);
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
