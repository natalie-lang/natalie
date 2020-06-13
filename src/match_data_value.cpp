#include "natalie.hpp"

namespace Natalie {

ssize_t MatchDataValue::index(ssize_t index) {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

Value *MatchDataValue::group(Env *env, ssize_t index) {
    if (index >= size()) return NAT_NIL;
    const char *str = &m_str[m_region->beg[index]];
    ssize_t length = m_region->end[index] - m_region->beg[index];
    return new StringValue { env, str, length };
}

}
