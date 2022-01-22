#pragma once

#include "natalie/gc.hpp"
#include "tm/string.hpp"

namespace Natalie {

using namespace TM;

// TODO: This object is here for legacy reasons and will be phased out.
// This is here so we can incrementally transition away from GC-managed strings in favor of TM::Strings.
class ManagedString : public Cell, public String {
public:
    using String::String;

    ManagedString(const ManagedString &other)
        : String { other } { }

    ManagedString(const String &other)
        : String { other } { }

    ManagedString &operator=(const ManagedString &other) {
        set_str(other.c_str(), other.length());
        return *this;
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<ManagedString %p str='%s'>", this, c_str());
    }

    ManagedString *clone() const { return new ManagedString { *this }; }
    ManagedString *substring(size_t start, size_t length) const { return new ManagedString { String::substring(start, length) }; }
    ManagedString *lowercase() const { return new ManagedString { String::lowercase() }; }
    ManagedString *uppercase() const { return new ManagedString { String::uppercase() }; }

    template <typename... Args>
    static ManagedString *format(const char *fmt, Args... args) {
        auto out = new ManagedString;
        format(out, fmt, args...);
        return out;
    }

    static void format(ManagedString *out, const char *fmt) {
        for (const char *c = fmt; *c != 0; c++) {
            out->append_char(*c);
        }
    }

    template <typename T, typename... Args>
    static void format(ManagedString *out, const char *fmt, T first, Args... rest) {
        for (const char *c = fmt; *c != 0; c++) {
            if (*c == '{' && *(c + 1) == '}') {
                c++;
                out->append(first);
                format(out, c + 1, rest...);
                return;
            } else {
                out->append_char(*c);
            }
        }
    }

    void append(const StringObject *str);

    // FIXME: why do I have to forward these? Shouldn't inheritance work for this?
    void append(signed char c) { String::append(c); }
    void append(unsigned char c) { String::append(c); }
    void append(size_t i) { String::append(i); }
    void append(long long i) { String::append(i); }
    void append(int i) { String::append(i); }
    void append(const char *str) { String::append(str); }
    void append(const String &str) { String::append(str); }
    void append(size_t n, char c) { String::append(n, c); }

    String to_low_level_string() const { return String(*this); }
};

}
