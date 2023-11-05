#include "natalie/array_packer/packer.hpp"

namespace Natalie {

namespace ArrayPacker {

    StringObject *Packer::pack(Env *env, StringObject *buffer) {

        m_packed = buffer->string();
        m_encoding = buffer->encoding();

        for (auto token : *m_directives) {
            if (token.error)
                env->raise("ArgumentError", *token.error);

            char d = token.directive;
            switch (d) {
            case 'A':
            case 'a':
            case 'B':
            case 'b':
            case 'H':
            case 'h':
            case 'M':
            case 'm':
            case 'P':
            case 'p':
            case 'u':
            case 'Z': {
                if (at_end())
                    env->raise("ArgumentError", "too few arguments");

                String string;
                StringObject *string_object = nullptr;
                auto item = m_source->at(m_index);
                if (m_source->is_string()) {
                    string_object = item->as_string();
                    string = item->as_string()->string();
                } else if (item->is_nil()) {
                    if (d == 'u' || d == 'm')
                        env->raise("TypeError", "no implicit conversion of nil into String");
                    string = "";
                } else if (item->respond_to(env, "to_str"_s)) {
                    string_object = item->to_str(env);
                    string = string_object->string();
                } else if (d == 'M' && (item->respond_to(env, "to_s"_s))) {
                    auto str = item->send(env, "to_s"_s);
                    if (str->is_string()) {
                        string_object = str->as_string();
                        string = str->as_string()->string();
                    } else {
                        string_object = str->to_s(env)->as_string();
                        string = str->to_s(env)->string();
                    }
                } else {
                    env->raise("TypeError", "no implicit conversion of {} into String", item->klass()->inspect_str());
                    NAT_UNREACHABLE();
                }

                auto packer = StringHandler { string, string_object, token };
                m_packed.append(packer.pack(env));

                if (d == 'm' || d == 'M' || d == 'u')
                    m_encoding = EncodingObject::get(Encoding::US_ASCII);

                m_index++;
                break;
            }
            case 'C':
            case 'c':
            case 'I':
            case 'i':
            case 'J':
            case 'j':
            case 'L':
            case 'l':
            case 'N':
            case 'n':
            case 'Q':
            case 'q':
            case 'S':
            case 's':
            case 'U':
            case 'V':
            case 'v':
            case 'w': {
                pack_with_loop(env, token, [&]() {
                    auto integer = m_source->at(m_index)->to_int(env);
                    auto packer = IntegerHandler { integer, token };
                    auto result = packer.pack(env);
                    m_packed.append(result);
                });

                if (d == 'U')
                    m_encoding = EncodingObject::get(Encoding::UTF_8);
                else
                    m_encoding = EncodingObject::get(Encoding::ASCII_8BIT);

                break;
            }
            case 'D':
            case 'd':
            case 'E':
            case 'e':
            case 'F':
            case 'f':
            case 'G':
            case 'g': {
                pack_with_loop(env, token, [&]() {
                    auto value = m_source->at(m_index);
                    if (value->is_integer()) {
                        value = value->as_integer()->to_f();
                    } else if (value->is_rational()) {
                        value = value->as_rational()->to_f(env);
                    }
                    auto packer = FloatHandler { value->as_float_or_raise(env), token };
                    m_packed.append(packer.pack(env));
                });
                break;
            }
            case 'x': {
                // Asterisks have no effect on this directive.
                if (token.star)
                    break;

                auto null_byte_count = (token.count < 0) ? 1 : token.count;
                for (int count = 0; count < null_byte_count; count++) {
                    m_packed.append_char('\0');
                }
                break;
            }
            case 'X': {
                // Asterisks have no effect on this directive.
                if (token.star)
                    break;

                auto amount_of_truncated_bytes = (token.count < 0) ? 1 : token.count;

                // If the packed string is empty or if the amount of bytes to be truncated
                // is greater than the packed string's current length,
                // then we can't truncate it. Raise ArgumentError.
                if (m_packed.length() == 0 || (int)m_packed.length() < amount_of_truncated_bytes) {
                    env->raise("ArgumentError", "X outside of the string");
                }
                m_packed.truncate(m_packed.length() - amount_of_truncated_bytes);
                break;
            }
            case '@': {
                auto count = (token.count < 0) ? 1 : token.count;
                auto missing_chars = static_cast<nat_int_t>(count) - static_cast<nat_int_t>(m_packed.size());
                if (missing_chars > 0) {
                    for (nat_int_t i = 0; i < missing_chars; ++i)
                        m_packed.append_char('\0');
                    break;
                }
                m_packed.truncate(count);
                break;
            }
            default: {
                env->raise("ArgumentError", "{} is not supported", d);
            }
            }
        }
        // must force str length in case m_packed was stuffed with '\0's
        buffer->set_str(m_packed.c_str(), m_packed.length());
        buffer->set_encoding(m_encoding);
        return buffer;
    }

}

}
