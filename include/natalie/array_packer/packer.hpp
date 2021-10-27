#pragma once

#include "natalie/array_packer/string_handler.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/string.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"

namespace Natalie {

namespace ArrayPacker {

    class Packer {
    public:
        Packer(ArrayValue *source, String *directives)
            : m_source { source }
            , m_directives { Tokenizer { directives }.tokenize() }
            , m_packed { new String } { }

        String *pack(Env *env) {
            signed char directive = 0;
            for (auto token : *m_directives) {
                if (token.error)
                    env->raise("ArgumentError", token.error);

                auto d = token.directive;
                switch (d) {
                case 'a':
                case 'A':
                case 'b':
                case 'B':
                case 'Z':
                case 'h':
                case 'H':
                case 'u': {
                    if (m_index >= m_source->size())
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

                    auto packer = StringHandler { string, token };
                    m_packed->append(packer.pack(env));

                    m_index++;
                    break;
                }
                case 'x':
                    // TODO
                    break;
                default: {
                    char buf[2] = { d, '\0' };
                    env->raise("StandardError", "unknown directive: {}", buf); // FIXME
                }
                }
            }
            return m_packed;
        }

    private:
        ArrayValue *m_source;
        TM::Vector<Token> *m_directives;
        String *m_packed;
        size_t m_index { 0 };
    };

}

}
