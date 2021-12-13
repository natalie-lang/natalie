#pragma once

#include "natalie/forward.hpp"
#include "natalie/types.hpp"

namespace Natalie {
class HashBuilder {
public:
    HashBuilder()
        : HashBuilder { 100019, true } { }

    HashBuilder(nat_int_t starting_digest, bool does_order_matter)
        : m_digest(starting_digest)
        , m_does_order_matter(does_order_matter) { }

    void append(nat_int_t hash) {
        if (m_does_order_matter)
            m_digest += (m_digest << 5) ^ hash;
        else
            m_digest ^= hash;
    }

    nat_int_t digest() const { return m_digest; }

private:
    nat_int_t m_digest;
    bool m_does_order_matter;
};

}
