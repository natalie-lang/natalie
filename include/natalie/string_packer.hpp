#pragma once

#include "natalie/env.hpp"
#include "natalie/string.hpp"

namespace Natalie {

class StringPacker {
public:
    StringPacker(String *source, String *directives, size_t directive_index)
        : m_source { source }
        , m_directives { directives }
        , m_packed { new String }
        , m_directive_index { directive_index } { }

    String *pack(Env *env) {
        signed char directive = 0;
        for (; m_directive_index < m_directives->length(); ++m_directive_index) {
            signed char d = (*m_directives)[m_directive_index];
            signed char next_d = m_directive_index + 1 < m_directives->length() ? (*m_directives)[m_directive_index + 1] : 0;
            bool star = next_d == '*';
            if (star) m_directive_index++;
            switch (d) {
            case 'a':
                directive = d;
                if (isdigit(next_d))
                    break;
                if (star) {
                    while (!at_end())
                        pack_a();
                } else {
                    pack_a();
                }
                break;
            case 'A':
                directive = d;
                if (isdigit(next_d))
                    break;
                if (star) {
                    while (!at_end())
                        pack_A();
                } else {
                    pack_A();
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                size_t count = (int)d - 48;
                // TODO consume additional digits?
                switch (directive) {
                case 0:
                    env->raise("StandardError", "no previous directive"); // FIXME
                    break;
                case 'a':
                    for (size_t i = 0; i < count; ++i)
                        pack_a();
                    break;
                case 'A':
                    for (size_t i = 0; i < count; ++i)
                        pack_A();
                    break;
                default:
                    env->raise("StandardError", "unknown directive (string)"); // FIXME
                }
                break;
            }
            case ' ':
            case '\t':
            case '\n':
            case '\v':
            case '\f':
            case '\r':
                // ignore whitespace
                break;
            default:
                env->raise("StandardError", "unknown directive (string)"); // FIXME
            }
        }

        return m_packed;
    }

    size_t directive_index() { return m_directive_index; }

private:
    void pack_a() {
        if (at_end())
            m_packed->append_char('\x00');
        else
            m_packed->append_char(m_source->at(m_index++));
    }

    void pack_A() {
        if (at_end())
            m_packed->append_char(' ');
        else
            m_packed->append_char(m_source->at(m_index++));
    }

    bool at_end() { return m_index >= m_source->length(); }

    String *m_source;
    String *m_directives;
    String *m_packed;
    size_t m_index { 0 };
    size_t m_directive_index { 0 };
};

}
