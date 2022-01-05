#pragma once

#include "natalie/array_object.hpp"
#include "natalie/array_packer/integer_handler.hpp"
#include "natalie/array_packer/string_handler.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include "natalie/env.hpp"
#include "natalie/string.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

namespace ArrayPacker {

    class Packer {
    public:
        Packer(ArrayObject *source, String *directives)
            : m_source { source }
            , m_directives { Tokenizer { directives }.tokenize() }
            , m_packed { new String }
            , m_encoding { Encoding::ASCII_8BIT } { }

        StringObject *pack(Env *env) {
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
                    if (at_end())
                        env->raise("ArgumentError", "too few arguments");

                    String *string;
                    auto item = m_source->at(m_index);
                    if (m_source->is_string()) {
                        string = item->as_string()->to_low_level_string();
                    } else if (item->is_nil()) {
                        if (d == 'u')
                            env->raise("TypeError", "no implicit conversion of nil into String");
                        string = new String("");
                    } else if (item->respond_to(env, "to_str"_s)) {
                        auto str = item->send(env, "to_str"_s);
                        str->assert_type(env, Object::Type::String, "String");
                        string = str->as_string()->to_low_level_string();
                    } else {
                        env->raise("TypeError", "no implicit conversion of {} into String", item->klass()->inspect_str());
                        NAT_UNREACHABLE();
                    }

                    auto packer = StringHandler { string, token };
                    m_packed->append(packer.pack(env));

                    m_index++;
                    break;
                }
                case 'C':
                case 'c':
                case 'U': {
                    pack_with_loop(env, token, [&]() {
                        IntegerObject *integer;
                        auto item = m_source->at(m_index);
                        if (item->is_nil()) { // TODO check if it is already implemented by the else branch at the end
                            env->raise("TypeError", "no implicit conversion of nil into Integer");
                        } else if (item->respond_to(env, "to_int"_s)) {
                            auto num = item->send(env, "to_int"_s);
                            num->assert_type(env, Object::Type::Integer, "Integer");
                            integer = num->as_integer();
                        } else {
                            env->raise("TypeError", "no implicit conversion of {} into Integer", item->klass()->inspect_str());
                            NAT_UNREACHABLE();
                        }

                        auto packer = IntegerHandler { integer, token };
                        m_packed->append(packer.pack(env));
                    });

                    if (d == 'U')
                        m_encoding = Encoding::UTF_8;
                    else
                        m_encoding = Encoding::ASCII_8BIT;

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
            return new StringObject { m_packed, m_encoding };
        }

    private:
        template <typename Fn>
        void pack_with_loop(Env *env, Token token, Fn handler) {
            auto starting_index = m_index;
            auto is_complete = [&]() {
                if (token.star)
                    return at_end();

                size_t count = token.count != -1 ? token.count : 1;
                bool is_complete = m_index - starting_index >= static_cast<size_t>(count);
                if (!is_complete && at_end())
                    env->raise("ArgumentError", "too few Arguments");
                return is_complete;
            };
            while (!is_complete()) {
                handler();
                m_index++;
            }
        }

        bool at_end() { return m_index >= m_source->size(); }

        ArrayObject *m_source;
        TM::Vector<Token> *m_directives;
        String *m_packed;
        Encoding m_encoding;
        size_t m_index { 0 };
    };

}

}
