#pragma once

#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/string.hpp"
#include "natalie/string_packer.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"

namespace Natalie {

class ArrayPacker {
public:
    ArrayPacker(ArrayValue *source, String *directives)
        : m_source { source }
        , m_directives { directives }
        , m_packed { new String } { }

    String *pack(Env *env) {
        validate(env);
        signed char directive = 0;
        for (size_t i = 0; i < m_directives->length(); ++i) {
            signed char d = (*m_directives)[i];
            //bool star = i + 1 < m_directives->length() && (*m_directives)[i + 1] == '*';
            //if (star) i++;
            switch (d) {
            case 'a':
            case 'A': {
                if (m_source->is_empty())
                    env->raise("ArgumentError", "too few arguments");

                String *string;
                auto item = m_source->at(m_index);
                if (m_source->is_string()) {
                    string = item->as_string()->to_low_level_string();
                } else if (item->is_nil()) {
                    string = new String("");
                } else if (item->respond_to(env, SymbolValue::intern("to_str"))) {
                    auto str = item->send(env, SymbolValue::intern("to_str"));
                    str->assert_type(env, Value::Type::String, "String");
                    string = str->as_string()->to_low_level_string();
                } else {
                    env->raise("TypeError", "no implicit conversion of {} into String", item->klass()->class_name_or_blank());
                    NAT_UNREACHABLE();
                }

                auto packer = StringPacker { string, m_directives, i };
                m_packed->append(packer.pack(env));
                i = packer.directive_index();
                break;
            }
            case 'x':
                // TODO
                break;
            default:
                env->raise("StandardError", "unknown directive"); // FIXME
            }
        }
        return m_packed;
    }

    void validate(Env *env) {
        signed char directive = 0;
        for (size_t i = 0; i < m_directives->length(); ++i) {
            signed char d = (*m_directives)[i];
            switch (d) {
            case 'a':
            case 'A':
                directive = d;
                break;
            case '_':
            case '!':
                if (directive != 's' && directive != 'S' && directive != 'i' && directive != 'I' && directive != 'l' && directive != 'L' && directive != 'q' && directive != 'Q' && directive != 'j' && directive != 'J') {
                    char buf[2] = { d, '\0' }; // FIXME: String::format needs some love :)
                    env->raise("ArgumentError", "'{}' allowed only after types sSiIlLqQjJ", buf);
                }
                break;
            default:
                break;
            }
        }
    }

private:
    ArrayValue *m_source;
    String *m_directives;
    String *m_packed;
    size_t m_index { 0 };
};

}
