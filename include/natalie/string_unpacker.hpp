#pragma once

#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

class StringUnpacker : public Cell {
    using Tokenizer = ArrayPacker::Tokenizer;
    using Token = ArrayPacker::Token;

public:
    StringUnpacker(const StringObject *source, String directives)
        : m_source { source }
        , m_directives { Tokenizer { directives }.tokenize() } { }

    ~StringUnpacker() { delete m_directives; }

    ArrayObject *unpack(Env *env) {
        for (auto token : *m_directives) {
            if (token.error)
                env->raise("ArgumentError", *token.error);

            switch (token.directive) {
            case 'i':
                unpack_i();
                break;
            case 'J':
                unpack_J();
                break;
            case 'P':
                unpack_P(token);
                break;
            case 'p':
                unpack_p();
                break;
            default:
                env->raise("ArgumentError", "{} is not supported", token.directive);
            }
        }
        return m_unpacked;
    }

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_source);
        visitor.visit(m_unpacked);
    }

private:
    void unpack_i() {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(Value::integer(*(int *)pointer()));
        m_index += sizeof(int);
    }

    void unpack_J() {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(Value::integer(*(uintptr_t *)pointer()));
        m_index += sizeof(uintptr_t);
    }

    void unpack_P(Token &token) {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer(), token.count));
        m_index += sizeof(uintptr_t);
    }

    void unpack_p() {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer()));
        m_index += sizeof(uintptr_t);
    }

    const char *pointer() { return m_source->c_str() + m_index; }

    bool at_end() { return m_index >= m_source->length(); }

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    ArrayObject *m_unpacked { new ArrayObject };
    size_t m_index { 0 };
};

}
