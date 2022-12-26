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

#define assert_str_eq(expected, actual)                                                   \
    {                                                                                     \
        auto e = (expected);                                                              \
        auto a = (actual);                                                                \
        if (a != e) {                                                                     \
            auto str = a.clone();                                                         \
            std::cerr << "\n"                                                             \
                      << "Expected \"" << str.c_str() << "\" to equal \"" << e << "\"\n"; \
            abort();                                                                      \
        }                                                                                 \
    }

#define assert_cstr_eq(expected, actual)                                        \
    {                                                                           \
        auto e = (expected);                                                    \
        auto a = (actual);                                                      \
        if (strcmp(a, e) != 0) {                                                \
            std::cerr << "\n"                                                   \
                      << "Expected \"" << e << "\" to equal \"" << a << "\"\n"; \
            abort();                                                            \
        }                                                                       \
    }

class Thing {
public:
    Thing() = default;

    Thing(const int value)
        : m_value { value } { }

    Thing(const Thing &other) = default;

    Thing(Thing &&other)
        : m_value { std::move(other.m_value) } {
        other.m_value = 0;
    }

    Thing &operator=(const int value) {
        m_value = value;
        return *this;
    }

    Thing &operator=(const Thing &other) = default;

    Thing &operator=(Thing &&other) {
        m_value = std::move(other.m_value);
        other.m_value = 0;
        return *this;
    }

    ~Thing() = default;

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
