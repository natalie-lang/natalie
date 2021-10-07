#pragma once

#include "natalie/big_int.hpp"
#include "natalie/integer_value.hpp"

namespace Natalie {

class BignumValue : public IntegerValue {
public:
    BignumValue(const String &num)
        : IntegerValue { -1 }
        , m_bignum(new BigInt(num)) { }

    BignumValue(const BigInt &other)
        : IntegerValue { -1 }
        , m_bignum(new BigInt(other)) { }

    ~BignumValue() {
        if (m_bignum) free(m_bignum);
    }

    ValuePtr add(Env *, ValuePtr) override;
    ValuePtr to_s(Env *, ValuePtr = nullptr) override;

    bool is_bignum() const override { return true; }
    BigInt to_bignum() const override { return *m_bignum; }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerValue %p bignum=%s>", this, m_bignum->to_string().c_str());
    }

private:
    BigInt *m_bignum { nullptr };
};
}
