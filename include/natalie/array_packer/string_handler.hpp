#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "tm/string.hpp"

namespace Natalie {

namespace ArrayPacker {

    class StringHandler : public Cell {
    public:
        StringHandler(String source, StringObject *string_object, Token token)
            : m_source { source }
            , m_string_object { string_object }
            , m_token { token } { }

        String pack(Env *env);

        using PackHandlerFn = void (StringHandler::*)();

        virtual void visit_children(Visitor &visitor) override {
            visitor.visit(m_string_object);
        }

    private:
        void pack_with_loop(PackHandlerFn handler);
        void pack_a();
        void pack_A();
        void pack_b();
        void pack_B();
        void pack_h();
        void pack_H();
        void pack_M();
        void pack_m();
        void pack_P();
        void pack_p();
        void pack_u();

        bool at_end() const { return m_index >= m_source.length(); }

        unsigned char next();

        // TODO: we can probably get rid of m_source since we have m_string_object now.
        String m_source {};
        StringObject *m_string_object { nullptr };
        Token m_token;
        String m_packed {};
        size_t m_index { 0 };
    };

}

}
