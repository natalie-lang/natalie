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

    BignumValue(const double &num)
        : IntegerValue { -1 }
        , m_bignum(new BigInt(num)) { }

    ~BignumValue() {
        if (m_bignum) delete m_bignum;
    }

    ValuePtr add(Env *, ValuePtr) override;
    ValuePtr sub(Env *, ValuePtr) override;
    ValuePtr mul(Env *, ValuePtr) override;
    ValuePtr div(Env *, ValuePtr) override;
    ValuePtr to_s(Env *, ValuePtr = nullptr) override;

    bool eq(Env *, ValuePtr) override;
    bool lt(Env *, ValuePtr) override;
    bool lte(Env *, ValuePtr) override;
    bool gt(Env *, ValuePtr) override;
    bool gte(Env *, ValuePtr) override;

    bool is_bignum() const override { return true; }
    BigInt to_bignum() const override { return *m_bignum; }

    bool has_to_be_bignum() const {
        return *m_bignum > MAX_INT || *m_bignum < MIN_INT;
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerValue %p bignum=%s>", this, m_bignum->to_string().c_str());
    }

private:
    static inline const BigInt MAX_INT = NAT_INT_MAX;
    static inline const BigInt MIN_INT = NAT_INT_MIN;
    BigInt *m_bignum { nullptr };
};
}
