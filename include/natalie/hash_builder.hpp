#pragma once

#include "natalie/forward.hpp"

namespace Natalie {
class HashBuilder {
public:
    HashBuilder()
        : HashBuilder { 100019 } { }

    HashBuilder(nat_int_t starting_digest)
        : m_digest(starting_digest) { }

    void append(nat_int_t hash) {
        m_digest += (m_digest << 5) ^ hash;
    }

    nat_int_t digest() { return m_digest; }

private:
    nat_int_t m_digest;
};

}