#pragma once

#include <iostream>
#include <string.h>

#define assert_eq(expected, actual)                                     \
    {                                                                   \
        auto e = (expected);                                            \
        auto a = (actual);                                              \
        if (a != e) {                                                   \
            std::cerr << "\n"                                           \
                      << "Expected " << a << " to equal " << e << "\n"; \
            abort();                                                    \
        }                                                               \
    }

#define assert_neq(a, b)                                                      \
    {                                                                         \
        auto a1 = (a);                                                        \
        auto b1 = (b);                                                        \
        if (a1 == b1) {                                                       \
            std::cerr << "\n"                                                 \
                      << "Expected " << a1 << " NOT to equal " << b1 << "\n"; \
            abort();                                                          \
        }                                                                     \
    }

#define assert_not(result)                                     \
    {                                                          \
        auto r = (result);                                     \
        if (r) {                                               \
            std::cerr << "\n"                                  \
                      << "Expected " << r << " to be false\n"; \
            abort();                                           \
        }                                                      \
    }

#define assert_str_eq(expected, actual)                                                 \
    {                                                                                   \
        auto e = (expected);                                                            \
        auto a = (actual);                                                              \
        if (a != e) {                                                                   \
            std::cerr << "\n"                                                           \
                      << "Expected \"" << a.c_str() << "\" to equal \"" << e << "\"\n"; \
            abort();                                                                    \
        }                                                                               \
    }

class Thing {
public:
    Thing() = default;

    Thing(int value)
        : m_value { value } { }

    Thing(const Thing &other)
        : m_value { other.m_value } { }

    ~Thing() { }

    bool operator==(const Thing &other) {
        return m_value == other.m_value;
    }

    bool operator!=(const Thing &other) {
        return m_value != other.m_value;
    }

    int value() const { return m_value; }

private:
    int m_value { 0 };
};

inline std::ostream &operator<<(std::ostream &os, const Thing &thing) {
    os << "Thing(" << thing.value() << ")";
    return os;
}
