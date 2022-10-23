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
        const char *pointer = m_source->c_str();
        auto final_null = m_source->c_str() + m_source->length();

        for (auto token : *m_directives) {
            if (token.error)
                env->raise("ArgumentError", *token.error);

            char d = token.directive;
            switch (d) {
            case 'i':
                if (pointer + sizeof(int) <= final_null)
                    m_unpacked->push(Value::integer(*(int *)pointer));
                else
                    m_unpacked->push(NilObject::the());
                pointer += sizeof(int);
                break;
            case 'J':
                if (pointer + sizeof(uintptr_t) <= final_null)
                    m_unpacked->push(Value::integer(*(uintptr_t *)pointer));
                else
                    m_unpacked->push(NilObject::the());
                pointer += sizeof(uintptr_t);
                break;
            case 'P':
                if (pointer + sizeof(uintptr_t) <= final_null)
                    m_unpacked->push(new StringObject(*(const char **)pointer, token.count));
                else
                    m_unpacked->push(NilObject::the());
                pointer += sizeof(uintptr_t);
                break;
            case 'p':
                if (pointer + sizeof(uintptr_t) <= final_null)
                    m_unpacked->push(new StringObject(*(const char **)pointer));
                else
                    m_unpacked->push(NilObject::the());
                pointer += sizeof(uintptr_t);
                break;
            default:
                env->raise("ArgumentError", "{} is not supported", d);
            }
        }
        return m_unpacked;
    }

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_source);
        visitor.visit(m_unpacked);
    }

private:
    bool at_end() { return m_index >= m_source->length(); }

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    ArrayObject *m_unpacked { new ArrayObject };
    size_t m_index { 0 };
};

}
