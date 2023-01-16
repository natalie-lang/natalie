#pragma once

#include "natalie/array_object.hpp"
#include "natalie/array_packer/float_handler.hpp"
#include "natalie/array_packer/integer_handler.hpp"
#include "natalie/array_packer/string_handler.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

namespace ArrayPacker {

    class Packer : public Cell {
    public:
        Packer(ArrayObject *source, String directives)
            : m_source { source }
            , m_directives { Tokenizer { directives }.tokenize() }
            , m_encoding { EncodingObject::get(Encoding::ASCII_8BIT) } { }

        ~Packer() { delete m_directives; }

        StringObject *pack(Env *env, StringObject *buffer);

        virtual void visit_children(Visitor &visitor) override {
            visitor.visit(m_source);
            visitor.visit(m_encoding);
        }

    private:
        template <typename Fn>
        void pack_with_loop(Env *env, Token token, Fn handler) {
            auto starting_index = m_index;

            auto is_complete = [&]() {
                if (token.star)
                    return at_end();

                size_t count = token.count != -1 ? token.count : 1;
                bool complete = m_index - starting_index >= count;

                if (!complete && at_end())
                    env->raise("ArgumentError", "too few Arguments");

                return complete;
            };

            while (!is_complete()) {
                handler();
                m_index++;
            }
        }

        bool at_end() const { return m_index >= m_source->size(); }

        ArrayObject *m_source;
        TM::Vector<Token> *m_directives;
        String m_packed {};
        EncodingObject *m_encoding;
        size_t m_index { 0 };
    };

}

}
