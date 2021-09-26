#pragma once

#include "natalie/env.hpp"
#include "natalie/string.hpp"

namespace Natalie {

class StringPacker {
public:
    StringPacker(String *source, String *directives)
        : m_source { source }
        , m_directives { directives }
        , m_packed { new String } { }

    String *pack(Env *env) {
        signed char directive = 0;
        for (size_t i = 0; i < m_directives->length(); ++i) {
            signed char d = (*m_directives)[i];
            bool star = i + 1 < m_directives->length() && (*m_directives)[i + 1] == '*';
            if (star) i++;
            switch (d) {
            case 'a':
                directive = d;
                if (star) {
                    while (!at_end())
                        pack_a();
                } else {
                    pack_a();
                }
                break;
            case 'A':
                directive = d;
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
                    for (size_t i = 0; i < count - 1; ++i)
                        pack_a();
                    break;
                case 'A':
                    for (size_t i = 0; i < count - 1; ++i)
                        pack_A();
                    break;
                default:
                    env->raise("StandardError", "unknown directive"); // FIXME
                }
                break;
            }
            default:
                env->raise("StandardError", "unknown directive"); // FIXME
            }
        }

        return m_packed;
    }

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
};

}
