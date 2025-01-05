#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Ibm437EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Ibm437EncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

void Ibm437EncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    str.append_sprintf("\\x%02llX", c);
}

nat_int_t Ibm437EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;

    auto result = to_unicode_map().get(codepoint);
    if (result == 0)
        return -1;

    return result;
}

nat_int_t Ibm437EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

String Ibm437EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t Ibm437EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

const TM::Hashmap<nat_int_t, nat_int_t> &Ibm437EncodingObject::to_unicode_map() {
    if (s_to_unicode_map.is_empty()) {
        s_to_unicode_map.put(0x80, 0xC7);
        s_to_unicode_map.put(0x81, 0xFC);
        s_to_unicode_map.put(0x82, 0xE9);
        s_to_unicode_map.put(0x83, 0xE2);
        s_to_unicode_map.put(0x84, 0xE4);
        s_to_unicode_map.put(0x85, 0xE0);
        s_to_unicode_map.put(0x86, 0xE5);
        s_to_unicode_map.put(0x87, 0xE7);
        s_to_unicode_map.put(0x88, 0xEA);
        s_to_unicode_map.put(0x89, 0xEB);
        s_to_unicode_map.put(0x8A, 0xE8);
        s_to_unicode_map.put(0x8B, 0xEF);
        s_to_unicode_map.put(0x8C, 0xEE);
        s_to_unicode_map.put(0x8D, 0xEC);
        s_to_unicode_map.put(0x8E, 0xC4);
        s_to_unicode_map.put(0x8F, 0xC5);
        s_to_unicode_map.put(0x90, 0xC9);
        s_to_unicode_map.put(0x91, 0xE6);
        s_to_unicode_map.put(0x92, 0xC6);
        s_to_unicode_map.put(0x93, 0xF4);
        s_to_unicode_map.put(0x94, 0xF6);
        s_to_unicode_map.put(0x95, 0xF2);
        s_to_unicode_map.put(0x96, 0xFB);
        s_to_unicode_map.put(0x97, 0xF9);
        s_to_unicode_map.put(0x98, 0xFF);
        s_to_unicode_map.put(0x99, 0xD6);
        s_to_unicode_map.put(0x9A, 0xDC);
        s_to_unicode_map.put(0x9B, 0xA2);
        s_to_unicode_map.put(0x9C, 0xA3);
        s_to_unicode_map.put(0x9D, 0xA5);
        s_to_unicode_map.put(0x9E, 0x20A7);
        s_to_unicode_map.put(0x9F, 0x192);
        s_to_unicode_map.put(0xA0, 0xE1);
        s_to_unicode_map.put(0xA1, 0xED);
        s_to_unicode_map.put(0xA2, 0xF3);
        s_to_unicode_map.put(0xA3, 0xFA);
        s_to_unicode_map.put(0xA4, 0xF1);
        s_to_unicode_map.put(0xA5, 0xD1);
        s_to_unicode_map.put(0xA6, 0xAA);
        s_to_unicode_map.put(0xA7, 0xBA);
        s_to_unicode_map.put(0xA8, 0xBF);
        s_to_unicode_map.put(0xA9, 0x2310);
        s_to_unicode_map.put(0xAA, 0xAC);
        s_to_unicode_map.put(0xAB, 0xBD);
        s_to_unicode_map.put(0xAC, 0xBC);
        s_to_unicode_map.put(0xAD, 0xA1);
        s_to_unicode_map.put(0xAE, 0xAB);
        s_to_unicode_map.put(0xAF, 0xBB);
        s_to_unicode_map.put(0xB0, 0x2591);
        s_to_unicode_map.put(0xB1, 0x2592);
        s_to_unicode_map.put(0xB2, 0x2593);
        s_to_unicode_map.put(0xB3, 0x2502);
        s_to_unicode_map.put(0xB4, 0x2524);
        s_to_unicode_map.put(0xB5, 0x2561);
        s_to_unicode_map.put(0xB6, 0x2562);
        s_to_unicode_map.put(0xB7, 0x2556);
        s_to_unicode_map.put(0xB8, 0x2555);
        s_to_unicode_map.put(0xB9, 0x2563);
        s_to_unicode_map.put(0xBA, 0x2551);
        s_to_unicode_map.put(0xBB, 0x2557);
        s_to_unicode_map.put(0xBC, 0x255D);
        s_to_unicode_map.put(0xBD, 0x255C);
        s_to_unicode_map.put(0xBE, 0x255B);
        s_to_unicode_map.put(0xBF, 0x2510);
        s_to_unicode_map.put(0xC0, 0x2514);
        s_to_unicode_map.put(0xC1, 0x2534);
        s_to_unicode_map.put(0xC2, 0x252C);
        s_to_unicode_map.put(0xC3, 0x251C);
        s_to_unicode_map.put(0xC4, 0x2500);
        s_to_unicode_map.put(0xC5, 0x253C);
        s_to_unicode_map.put(0xC6, 0x255E);
        s_to_unicode_map.put(0xC7, 0x255F);
        s_to_unicode_map.put(0xC8, 0x255A);
        s_to_unicode_map.put(0xC9, 0x2554);
        s_to_unicode_map.put(0xCA, 0x2569);
        s_to_unicode_map.put(0xCB, 0x2566);
        s_to_unicode_map.put(0xCC, 0x2560);
        s_to_unicode_map.put(0xCD, 0x2550);
        s_to_unicode_map.put(0xCE, 0x256C);
        s_to_unicode_map.put(0xCF, 0x2567);
        s_to_unicode_map.put(0xD0, 0x2568);
        s_to_unicode_map.put(0xD1, 0x2564);
        s_to_unicode_map.put(0xD2, 0x2565);
        s_to_unicode_map.put(0xD3, 0x2559);
        s_to_unicode_map.put(0xD4, 0x2558);
        s_to_unicode_map.put(0xD5, 0x2552);
        s_to_unicode_map.put(0xD6, 0x2553);
        s_to_unicode_map.put(0xD7, 0x256B);
        s_to_unicode_map.put(0xD8, 0x256A);
        s_to_unicode_map.put(0xD9, 0x2518);
        s_to_unicode_map.put(0xDA, 0x250C);
        s_to_unicode_map.put(0xDB, 0x2588);
        s_to_unicode_map.put(0xDC, 0x2584);
        s_to_unicode_map.put(0xDD, 0x258C);
        s_to_unicode_map.put(0xDE, 0x2590);
        s_to_unicode_map.put(0xDF, 0x2580);
        s_to_unicode_map.put(0xE0, 0x3B1);
        s_to_unicode_map.put(0xE1, 0xDF);
        s_to_unicode_map.put(0xE2, 0x393);
        s_to_unicode_map.put(0xE3, 0x3C0);
        s_to_unicode_map.put(0xE4, 0x3A3);
        s_to_unicode_map.put(0xE5, 0x3C3);
        s_to_unicode_map.put(0xE6, 0xB5);
        s_to_unicode_map.put(0xE7, 0x3C4);
        s_to_unicode_map.put(0xE8, 0x3A6);
        s_to_unicode_map.put(0xE9, 0x398);
        s_to_unicode_map.put(0xEA, 0x3A9);
        s_to_unicode_map.put(0xEB, 0x3B4);
        s_to_unicode_map.put(0xEC, 0x221E);
        s_to_unicode_map.put(0xED, 0x3C6);
        s_to_unicode_map.put(0xEE, 0x3B5);
        s_to_unicode_map.put(0xEF, 0x2229);
        s_to_unicode_map.put(0xF0, 0x2261);
        s_to_unicode_map.put(0xF1, 0xB1);
        s_to_unicode_map.put(0xF2, 0x2265);
        s_to_unicode_map.put(0xF3, 0x2264);
        s_to_unicode_map.put(0xF4, 0x2320);
        s_to_unicode_map.put(0xF5, 0x2321);
        s_to_unicode_map.put(0xF6, 0xF7);
        s_to_unicode_map.put(0xF7, 0x2248);
        s_to_unicode_map.put(0xF8, 0xB0);
        s_to_unicode_map.put(0xF9, 0x2219);
        s_to_unicode_map.put(0xFA, 0xB7);
        s_to_unicode_map.put(0xFB, 0x221A);
        s_to_unicode_map.put(0xFC, 0x207F);
        s_to_unicode_map.put(0xFD, 0xB2);
        s_to_unicode_map.put(0xFE, 0x25A0);
        s_to_unicode_map.put(0xFF, 0xA0);
    }
    return s_to_unicode_map;
}

}
