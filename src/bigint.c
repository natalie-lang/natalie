#include "natalie/bigint.h"

// https://github.com/983/bigint

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIGINT_ASSERT(a, op, b) assert((a)op(b));

// temporary
void print_bigint(const bigint *n) {
    printf("size = %d, cap = %d, neg = %d\n", n->size, n->capacity, n->neg);
    for (int i = 0; i < n->size; i++) {
        printf("words[%d] = %u\n", i, n->words[i]);
    }
}

/* low bits of a * b */
bigint_word bigint_word_mul_lo(bigint_word a, bigint_word b) {
    return a * b;
}

/* high bits of a * b */
bigint_word bigint_word_mul_hi(bigint_word a, bigint_word b) {
    bigint_word c0 = BIGINT_WORD_LO(a) * BIGINT_WORD_LO(b);
    bigint_word c1 = BIGINT_WORD_LO(a) * BIGINT_WORD_HI(b);
    bigint_word c2 = BIGINT_WORD_HI(a) * BIGINT_WORD_LO(b);
    bigint_word c3 = BIGINT_WORD_HI(a) * BIGINT_WORD_HI(b);

    bigint_word c4 = BIGINT_WORD_HI(c0) + BIGINT_WORD_LO(c1) + BIGINT_WORD_LO(c2);
    return BIGINT_WORD_HI(c4) + BIGINT_WORD_HI(c1) + BIGINT_WORD_HI(c2) + c3;
}

/* dst = a + b, return carry */
bigint_word bigint_word_add_get_carry(
    bigint_word *dst,
    bigint_word a,
    bigint_word b) {
    a += b;
    *dst = a;
    return a < b;
}

/* dst = a - b, return carry */
bigint_word bigint_word_sub_get_carry(
    bigint_word *dst,
    bigint_word a,
    bigint_word b) {
    b = a - b;
    *dst = b;
    return b > a;
}

bigint_word bigint_word_from_char(char c) {
    switch (c) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    case 'g':
    case 'G':
        return 16;
    case 'h':
    case 'H':
        return 17;
    case 'i':
    case 'I':
        return 18;
    case 'j':
    case 'J':
        return 19;
    case 'k':
    case 'K':
        return 20;
    case 'l':
    case 'L':
        return 21;
    case 'm':
    case 'M':
        return 22;
    case 'n':
    case 'N':
        return 23;
    case 'o':
    case 'O':
        return 24;
    case 'p':
    case 'P':
        return 25;
    case 'q':
    case 'Q':
        return 26;
    case 'r':
    case 'R':
        return 27;
    case 's':
    case 'S':
        return 28;
    case 't':
    case 'T':
        return 29;
    case 'u':
    case 'U':
        return 30;
    case 'v':
    case 'V':
        return 31;
    case 'w':
    case 'W':
        return 32;
    case 'x':
    case 'X':
        return 33;
    case 'y':
    case 'Y':
        return 34;
    case 'z':
    case 'Z':
        return 35;
    default:
        return BIGINT_WORD_MAX;
    }
}

int bigint_word_bitlength(bigint_word a) {
    int i;
    for (i = BIGINT_WORD_BITS - 1; i >= 0; i--)
        if ((a >> i) & 1) return i + 1;
    return 0;
}

int bigint_word_count_trailing_zeros(bigint_word a) {
    int i;
    for (i = 0; i < (int)BIGINT_WORD_BITS; i++)
        if ((a >> i) & 1) return i;
    return BIGINT_WORD_BITS;
}

bigint_word bigint_word_gcd(bigint_word a, bigint_word b) {
    while (1) {
        if (!a) return b;
        b %= a;
        if (!b) return a;
        a %= b;
    }
}

unsigned bigint_uint_gcd(unsigned a, unsigned b) {
    while (1) {
        if (!a) return b;
        b %= a;
        if (!b) return a;
        a %= b;
    }
}

int bigint_int_gcd(int a, int b) {
    return bigint_uint_gcd(BIGINT_INT_ABS(a), BIGINT_INT_ABS(b));
}

bigint *bigint_init(bigint *dst) {
    dst->words = NULL;
    dst->neg = dst->size = dst->capacity = 0;
    return dst;
}

bigint *bigint_reserve(bigint *dst, int capacity) {
    BIGINT_ASSERT(capacity, >, 0);
    if (dst->capacity >= capacity) return dst;
    dst->capacity = capacity;
    dst->words = (bigint_word *)realloc(dst->words, capacity * sizeof(*dst->words));
    /* out of memory? sorry :( */
    assert(dst->words != NULL);
    BIGINT_ASSERT(dst->size, <=, capacity);
    return dst;
}

void bigint_free(bigint *dst) {
    free(dst->words);
    bigint_init(dst);
}

int bigint_raw_cmp_abs(
    const bigint_word *a, int na,
    const bigint_word *b, int nb) {
    int i;

    if (na > nb) return +1;
    if (na < nb) return -1;

    BIGINT_ASSERT(na, ==, nb);
    for (i = na - 1; i >= 0; i--) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return +1;
    }

    return 0;
}

int bigint_cmp_abs(const bigint *a, const bigint *b) {
    return bigint_raw_cmp_abs(a->words, a->size, b->words, b->size);
}

int bigint_raw_cmp(
    const bigint_word *a, int na, int a_neg,
    const bigint_word *b, int nb, int b_neg) {
    if (na == 0 && nb == 0) return 0;

    if (!a_neg && !b_neg) return bigint_raw_cmp_abs(a, na, b, nb);
    if (a_neg && b_neg) return bigint_raw_cmp_abs(a, na, b, nb) * -1;

    return (!a_neg && b_neg) ? +1 : -1;
}

int bigint_cmp(const bigint *a, const bigint *b) {
    return bigint_raw_cmp(a->words, a->size, a->neg, b->words, b->size, b->neg);
}

int bigint_cmp_abs_word(const bigint *a, bigint_word b) {
    if (b == 0) {
        if (a->size == 0) return 0;
        return a->neg ? -1 : +1;
    }
    return bigint_raw_cmp_abs(a->words, a->size, &b, 1);
}

void bigint_raw_zero(bigint_word *dst, int from, int to) {
    if (from >= to) return;
    memset(dst + from, 0, (to - from) * sizeof(*dst)); // NOLINT
}

bigint *bigint_set_neg(bigint *dst, int neg) {
    dst->neg = neg;
    return dst;
}

bigint *bigint_negate(bigint *dst) {
    return bigint_set_neg(dst, !dst->neg);
}

int bigint_raw_cpy(bigint_word *dst, const bigint_word *src, int n) {
    memcpy(dst, src, n * sizeof(*src)); // NOLINT
    return n;
}

bigint *bigint_cpy(bigint *dst, const bigint *src) {
    if (src == dst) return dst;
    if (src->size == 0) {
        dst->size = 0;
        return bigint_set_neg(dst, src->neg);
    }

    bigint_reserve(dst, src->size);
    dst->size = bigint_raw_cpy(dst->words, src->words, src->size);
    BIGINT_ASSERT(bigint_cmp_abs(src, dst), ==, 0);
    return bigint_set_neg(dst, src->neg);
}

int bigint_raw_truncate(const bigint_word *a, int n) {
    while (n > 0 && a[n - 1] == 0)
        n--;
    return n;
}

void bigint_raw_clr_bit(bigint_word *dst, unsigned bit_index) {
    unsigned word_index = bit_index / BIGINT_WORD_BITS;
    bit_index %= BIGINT_WORD_BITS;
    dst[word_index] &= BIGINT_WORD_MAX ^ (((bigint_word)1) << bit_index);
}

bigint *bigint_clr_bit(bigint *dst, unsigned bit_index) {
    bigint_raw_clr_bit(dst->words, bit_index);
    dst->size = bigint_raw_truncate(dst->words, dst->size);
    return dst;
}

bigint *bigint_set_bit(bigint *dst, unsigned bit_index) {
    int word_index = bit_index / BIGINT_WORD_BITS;
    int n = word_index + 1;

    bigint_reserve(dst, n);
    bigint_raw_zero(dst->words, dst->size, n);
    dst->size = BIGINT_MAX(dst->size, n);
    dst->words[word_index] |= ((bigint_word)1) << bit_index % BIGINT_WORD_BITS;

    return dst;
}

bigint_word bigint_get_bit(const bigint *src, unsigned bit_index) {
    int i = bit_index / BIGINT_WORD_BITS;

    if (src->size <= i) return 0;

    return (src->words[i] >> bit_index % BIGINT_WORD_BITS) & 1;
}

int bigint_raw_mul_word_add(
    bigint_word *dst,
    const bigint_word *src, int n,
    bigint_word factor) {
    int i;
    bigint_word carry = 0;

    for (i = 0; i < n; i++) {
        bigint_word src_word = src[i];
        bigint_word dst_word = bigint_word_mul_lo(src_word, factor);
        carry = bigint_word_add_get_carry(&dst_word, dst_word, carry);
        carry += bigint_word_mul_hi(src_word, factor);
        carry += bigint_word_add_get_carry(&dst[i], dst[i], dst_word);
    }

    for (; carry; i++) {
        carry = bigint_word_add_get_carry(&dst[i], dst[i], carry);
    }

    return bigint_raw_truncate(dst, i);
}

int bigint_raw_mul_word(
    bigint_word *dst,
    const bigint_word *src, int n,
    bigint_word factor) {
    int i;
    bigint_word carry = 0;

    for (i = 0; i < n; i++) {
        bigint_word src_word = src[i];
        bigint_word dst_word = bigint_word_mul_lo(src_word, factor);
        carry = bigint_word_add_get_carry(&dst_word, dst_word, carry);
        carry += bigint_word_mul_hi(src_word, factor);
        dst[i] = dst_word;
    }

    if (carry) {
        dst[i++] = carry;
    }

    return bigint_raw_truncate(dst, i);
}

int bigint_raw_mul_add(
    bigint_word *dst,
    const bigint_word *src_a, int na,
    const bigint_word *src_b, int nb) {
    int i;

    if (na == 0 || nb == 0) return 0;

    assert(dst != src_a);
    assert(dst != src_b);

    for (i = 0; i < nb; i++) {
        bigint_raw_mul_word_add(dst + i, src_a, na, src_b[i]);
    }

    return bigint_raw_truncate(dst, na + nb);
}

int bigint_raw_add_word(
    bigint_word *dst,
    const bigint_word *src, int n,
    bigint_word b) {
    int i;
    bigint_word carry = b;

    for (i = 0; i < n; i++) {
        carry = bigint_word_add_get_carry(&dst[i], src[i], carry);
    }

    for (; carry; i++) {
        carry = bigint_word_add_get_carry(&dst[i], dst[i], carry);
    }

    return bigint_raw_truncate(dst, i);
}

int bigint_raw_from_str_base(bigint_word *dst, const char *src, int base) {
    int n = 0;

    for (; *src; src++) {
        bigint_word digit = bigint_word_from_char(*src);

        if (digit == BIGINT_WORD_MAX) continue;

        n = bigint_raw_mul_word(dst, dst, n, base);
        n = bigint_raw_add_word(dst, dst, n, digit);
    }

    return bigint_raw_truncate(dst, n);
}

int bigint_count_digits(const char *src) {
    int n = 0;
    for (; *src; src++)
        if (bigint_word_from_char(*src) != BIGINT_WORD_MAX) n++;
    return n;
}

int bigint_raw_add(
    bigint_word *dst,
    const bigint_word *src_a, int na,
    const bigint_word *src_b, int nb) {
    bigint_word sum, carry = 0;
    int i, n = BIGINT_MIN(na, nb);

    for (i = 0; i < n; i++) {
        carry = bigint_word_add_get_carry(&sum, carry, src_a[i]);
        carry += bigint_word_add_get_carry(&sum, sum, src_b[i]);
        dst[i] = sum;
    }

    for (; i < na; i++) {
        carry = bigint_word_add_get_carry(&dst[i], src_a[i], carry);
    }

    for (; i < nb; i++) {
        carry = bigint_word_add_get_carry(&dst[i], src_b[i], carry);
    }

    if (carry) dst[i++] = carry;

    return bigint_raw_truncate(dst, i);
}

int bigint_raw_sub(
    bigint_word *dst,
    const bigint_word *src_a, int na,
    const bigint_word *src_b, int nb) {
    bigint_word dif, carry = 0;
    int i;
    BIGINT_ASSERT(na, >=, nb);
    BIGINT_ASSERT(bigint_raw_cmp_abs(src_a, na, src_b, nb), >=, 0);

    for (i = 0; i < nb; i++) {
        carry = bigint_word_sub_get_carry(&dif, src_a[i], carry);
        carry += bigint_word_sub_get_carry(&dif, dif, src_b[i]);
        dst[i] = dif;
    }

    for (; i < na; i++) {
        carry = bigint_word_sub_get_carry(&dst[i], src_a[i], carry);
    }

    BIGINT_ASSERT(carry, ==, 0);
    return bigint_raw_truncate(dst, i);
}

int bigint_raw_mul_karatsuba(
    bigint_word *dst,
    const bigint_word *a, int na,
    const bigint_word *b, int nb,
    bigint_word *tmp) {
    /* so many */
    int n, k, m, m2;
    const bigint_word *lo1, *hi1, *lo2, *hi2;
    int nlo1, nhi1, nlo2, nhi2;
    bigint_word *lo1hi1, *lo2hi2, *z0, *z1, *z2;
    int nlo1hi1, nlo2hi2, nz0, nz1, nz2;

    if (
        na < BIGINT_KARATSUBA_WORD_THRESHOLD && nb < BIGINT_KARATSUBA_WORD_THRESHOLD) {
        bigint_raw_zero(dst, 0, na + nb);
        return bigint_raw_mul_add(dst, a, na, b, nb);
    }

    m = BIGINT_MAX(na, nb);
    m2 = m / 2;
    k = m2 + 2;

    lo1 = a;
    lo2 = b;
    hi1 = a + m2;
    hi2 = b + m2;

    nlo1 = bigint_raw_truncate(lo1, BIGINT_MIN(m2, na));
    nlo2 = bigint_raw_truncate(lo2, BIGINT_MIN(m2, nb));
    nhi1 = bigint_raw_truncate(hi1, BIGINT_MAX(na - m2, 0));
    nhi2 = bigint_raw_truncate(hi2, BIGINT_MAX(nb - m2, 0));

    lo1hi1 = tmp;
    tmp += k;
    lo2hi2 = tmp;
    tmp += k;
    z0 = tmp;
    tmp += k * 2;
    z1 = tmp;
    tmp += k * 2;
    z2 = tmp;
    tmp += k * 2;

    nlo1hi1 = bigint_raw_add(lo1hi1, lo1, nlo1, hi1, nhi1);
    nlo2hi2 = bigint_raw_add(lo2hi2, lo2, nlo2, hi2, nhi2);

    nz0 = bigint_raw_mul_karatsuba(z0, lo1, nlo1, lo2, nlo2, tmp);
    nz1 = bigint_raw_mul_karatsuba(z1, lo1hi1, nlo1hi1, lo2hi2, nlo2hi2, tmp);
    nz2 = bigint_raw_mul_karatsuba(z2, hi1, nhi1, hi2, nhi2, tmp);

    nz1 = bigint_raw_sub(z1, z1, nz1, z0, nz0);
    nz1 = bigint_raw_sub(z1, z1, nz1, z2, nz2);

    n = nz0;

    bigint_raw_cpy(dst, z0, n);
    bigint_raw_zero(dst, n, na + nb);

    n = bigint_raw_add(dst + m2 * 1, dst + m2 * 1, BIGINT_MAX(n - m2, 0), z1, nz1);
    n = bigint_raw_add(dst + m2 * 2, dst + m2 * 2, BIGINT_MAX(n - m2, 0), z2, nz2);

    return bigint_raw_truncate(dst, n + m2 * 2);
}

bigint *bigint_mul(bigint *dst, const bigint *a, const bigint *b) {
    int na = a->size;
    int nb = b->size;
    int n = na + nb;
    bigint_word *tmp;

    bigint_reserve(dst, n);

    /* bigint_raw_mul_karatsuba already has this fastpath */
    /* but this way we avoid allocating tmp */
    if (
        dst != a && dst != b && na < BIGINT_KARATSUBA_WORD_THRESHOLD && nb < BIGINT_KARATSUBA_WORD_THRESHOLD) {
        bigint_raw_zero(dst->words, 0, na + nb);
        dst->size = bigint_raw_mul_add(dst->words, a->words, na, b->words, nb);
    } else {
        int magical_upper_bound = BIGINT_MAX(na, nb) * 11 + 180 + n;
        tmp = (bigint_word *)malloc(magical_upper_bound * sizeof(*tmp));

        dst->size = bigint_raw_mul_karatsuba(tmp, a->words, na, b->words, nb, tmp + n);
        bigint_raw_cpy(dst->words, tmp, dst->size);
        free(tmp);
    }

    return bigint_set_neg(dst, a->neg ^ b->neg);
}

int bigint_digits_bound(int n_digits_src, double src_base, double dst_base) {
    /* +1 for rounding errors, just in case */
    return ceil(n_digits_src * log(src_base) / log(dst_base)) + 1;
}

int bigint_write_size(const bigint *a, double dst_base) {
    double src_base = pow(2, BIGINT_WORD_BITS);
    return bigint_digits_bound(a->size, src_base, dst_base)
        + sizeof('-') + sizeof('\0');
}

bigint *bigint_from_str_base(bigint *dst, const char *src, int src_base) {
    int n_digits_src, n_digits_dst;
    double dst_base = pow(2.0, BIGINT_WORD_BITS);

    n_digits_src = bigint_count_digits(src);
    n_digits_dst = bigint_digits_bound(n_digits_src, src_base, dst_base);

    bigint_reserve(dst, n_digits_dst);
    dst->size = n_digits_dst;
    bigint_raw_zero(dst->words, 0, n_digits_dst);

    dst->size = bigint_raw_from_str_base(dst->words, src, src_base);
    return bigint_set_neg(dst, *src == '-');
}

bigint *bigint_from_str(bigint *dst, const char *src) {
    return bigint_from_str_base(dst, src, 10);
}

bigint *bigint_from_int(bigint *dst, int src) {
    unsigned int x = BIGINT_INT_ABS(src);
    int n = BIGINT_MAX(1, sizeof(x) / sizeof(bigint_word));
    bigint_reserve(dst, n);
    bigint_raw_zero(dst->words, 0, n);
    memcpy(dst->words, &x, sizeof(x)); // NOLINT
    dst->size = bigint_raw_truncate(dst->words, n);
    return bigint_set_neg(dst, src < 0);
}

bigint *bigint_from_word(bigint *dst, bigint_word a) {
    bigint_reserve(dst, 1);
    dst->words[0] = a;
    dst->size = bigint_raw_truncate(dst->words, 1);
    return bigint_set_neg(dst, 0);
}

int bigint_raw_add_signed(
    bigint_word *dst, int *dst_neg,
    const bigint_word *a, int na, int a_neg,
    const bigint_word *b, int nb, int b_neg) {
    if (a_neg) {
        if (b_neg) {
            if (na >= nb) {
                *dst_neg = 1;
                return bigint_raw_add(dst, a, na, b, nb);
            } else {
                *dst_neg = 1;
                return bigint_raw_add(dst, b, nb, a, na);
            }
        } else {
            if (bigint_raw_cmp_abs(a, na, b, nb) >= 0) {
                *dst_neg = 1;
                return bigint_raw_sub(dst, a, na, b, nb);
            } else {
                *dst_neg = 0;
                return bigint_raw_sub(dst, b, nb, a, na);
            }
        }
    } else {
        if (b_neg) {
            if (bigint_raw_cmp_abs(a, na, b, nb) >= 0) {
                *dst_neg = 0;
                return bigint_raw_sub(dst, a, na, b, nb);
            } else {
                *dst_neg = 1;
                return bigint_raw_sub(dst, b, nb, a, na);
            }
        } else {
            if (na >= nb) {
                *dst_neg = 0;
                return bigint_raw_add(dst, a, na, b, nb);
            } else {
                *dst_neg = 0;
                return bigint_raw_add(dst, b, nb, a, na);
            }
        }
    }
}

bigint *bigint_add_signed(
    bigint *dst,
    const bigint *a, int a_neg,
    const bigint *b, int b_neg) {
    int na = a->size;
    int nb = b->size;
    int n = BIGINT_MAX(na, nb) + 1;

    bigint_reserve(dst, n);

    dst->size = bigint_raw_add_signed(
        dst->words, &dst->neg,
        a->words, na, a_neg,
        b->words, nb, b_neg);

    return dst;
}

bigint *bigint_add(bigint *dst, const bigint *a, const bigint *b) {
    return bigint_add_signed(dst, a, a->neg, b, b->neg);
}

bigint *bigint_sub(bigint *dst, const bigint *a, const bigint *b) {
    return bigint_add_signed(dst, a, a->neg, b, !b->neg);
}

bigint *bigint_add_word_signed(
    bigint *dst,
    const bigint *src_a,
    bigint_word b, int b_neg) {
    int na = src_a->size;

    bigint_reserve(dst, na + 1);

    dst->size = bigint_raw_add_signed(
        dst->words, &dst->neg,
        src_a->words, na, src_a->neg,
        &b, 1, b_neg);

    return dst;
}

bigint *bigint_add_word(bigint *dst, const bigint *src_a, bigint_word b) {
    return bigint_add_word_signed(dst, src_a, b, 0);
}

bigint *bigint_sub_word(bigint *dst, const bigint *src_a, bigint_word b) {
    return bigint_add_word_signed(dst, src_a, b, 1);
}

int bigint_raw_shift_left(
    bigint_word *dst, int n_dst,
    const bigint_word *src, int n_src,
    unsigned long long shift) {
    int i;
    int word_shift = shift / BIGINT_WORD_BITS;
    int bits_shift = shift % BIGINT_WORD_BITS;

    if (bits_shift) {
        bigint_word lo, hi = 0;

        for (i = n_src + word_shift; i > word_shift; i--) {
            lo = src[i - word_shift - 1];
            BIGINT_ASSERT(i, >=, 0);
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = (hi << bits_shift) | (lo >> (BIGINT_WORD_BITS - bits_shift));
            hi = lo;
        }

        for (i = word_shift; i >= 0; i--) {
            BIGINT_ASSERT(i, >=, 0);
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = hi << bits_shift;
            hi = 0;
        }

        i = n_src + word_shift + 1;
        BIGINT_ASSERT(i, <=, n_dst);
        return bigint_raw_truncate(dst, i);
    } else {
        /* this case is not only separate because of performance */
        /* but (lo >> (BIGINT_WORD_BITS - 0)) is also undefined behaviour */
        for (i = n_src + word_shift - 1; i >= word_shift; i--) {
            BIGINT_ASSERT(i, >=, 0);
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = src[i - word_shift];
        }

        for (i = word_shift - 1; i >= 0; i--) {
            BIGINT_ASSERT(i, >=, 0);
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = 0;
        }

        i = n_src + word_shift;
        BIGINT_ASSERT(i, <=, n_dst);
        return bigint_raw_truncate(dst, i);
    }
}

int bigint_raw_shift_right(
    bigint_word *dst, int n_dst,
    const bigint_word *src, int n_src,
    int shift) {
    int i;
    int word_shift = shift / BIGINT_WORD_BITS;
    int bits_shift = shift % BIGINT_WORD_BITS;

    if (bits_shift) {
        bigint_word hi, lo = src[word_shift];

        for (i = 0; i < n_src - word_shift - 1; i++) {
            hi = src[i + word_shift + 1];
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = (hi << (BIGINT_WORD_BITS - bits_shift)) | (lo >> bits_shift);
            lo = hi;
        }

        BIGINT_ASSERT(i, <, n_dst);
        dst[i++] = lo >> bits_shift;

        BIGINT_ASSERT(i, <=, n_dst);
        return bigint_raw_truncate(dst, i);
    } else {
        /* this case is not only separate because of performance */
        /* but (hi << (BIGINT_WORD_BITS - 0)) is also undefined behaviour */
        for (i = 0; i < n_src - word_shift; i++) {
            BIGINT_ASSERT(i, <, n_dst);
            dst[i] = src[i + word_shift];
        }

        BIGINT_ASSERT(i, <=, n_dst);
        return bigint_raw_truncate(dst, i);
    }
}

bigint *bigint_shift_left(bigint *dst, const bigint *src, unsigned long long shift) {
    unsigned n = src->size + shift / BIGINT_WORD_BITS + (shift % BIGINT_WORD_BITS != 0);
    bigint_reserve(dst, n);
    dst->size = bigint_raw_shift_left(dst->words, dst->capacity, src->words, src->size, shift);
    return bigint_set_neg(dst, src->neg);
}

bigint *bigint_shift_right(bigint *dst, const bigint *src, unsigned long long shift) {
    bigint_reserve(dst, src->size);
    dst->size = bigint_raw_shift_right(dst->words, dst->capacity, src->words, src->size, shift);
    return bigint_set_neg(dst, src->neg);
}

int bigint_raw_bitlength(const bigint_word *src_a, int na) {
    int last = na - 1;
    if (last < 0) return 0;
    return bigint_word_bitlength(src_a[last]) + last * BIGINT_WORD_BITS;
}

int bigint_bitlength(const bigint *a) {
    return bigint_raw_bitlength(a->words, a->size);
}

int bigint_count_trailing_zeros(const bigint *a) {
    int i;
    for (i = 0; i < a->size; i++) {
        bigint_word w = a->words[i];
        if (w) return bigint_word_count_trailing_zeros(w) + i * BIGINT_WORD_BITS;
    }
    return a->size * BIGINT_WORD_BITS;
}

bigint *bigint_div_mod(
    bigint *dst_quotient,
    bigint *dst_remainder,
    const bigint *src_numerator,
    const bigint *src_denominator) {
    int shift;
    int src_numerator_neg = src_numerator->neg;
    int src_denominator_neg = src_denominator->neg;
    bigint denominator[1], *remainder = dst_remainder, *quotient = dst_quotient;

    if (src_denominator->size == 0) return NULL;

    /* fast path for native word size */
    if (src_numerator->size == 1 && src_denominator->size == 1) {
        /* make sure this is not overwritten */
        bigint_word a = src_numerator->words[0];
        bigint_word b = src_denominator->words[0];
        bigint_from_word(quotient, a / b);
        bigint_from_word(remainder, a % b);
        quotient->neg = src_numerator_neg ^ src_denominator_neg;
        remainder->neg = src_numerator_neg;
        return dst_quotient;
    }

    /* fast path for half word size */
    if (src_denominator->size == 1 && src_denominator->words[0] <= BIGINT_HALF_WORD_MAX) {
        bigint_word rem;
        bigint_cpy(quotient, src_numerator);
        bigint_div_mod_half_word(quotient, &rem, src_denominator->words[0]);
        bigint_from_word(remainder, rem);
        quotient->neg = src_numerator_neg ^ src_denominator_neg;
        remainder->neg = src_numerator_neg;
        return dst_quotient;
    }

    bigint_cpy(remainder, src_numerator);
    remainder->neg = 0;
    quotient->size = 0;

    if (bigint_cmp_abs(remainder, src_denominator) >= 0) {
        shift = bigint_bitlength(remainder) - bigint_bitlength(src_denominator);

        bigint_init(denominator);
        bigint_shift_left(denominator, src_denominator, shift);
        denominator->neg = 0;

        /* divide bit by bit */
        for (; shift >= 0; shift--) {
            if (bigint_cmp_abs(remainder, denominator) >= 0) {
                bigint_sub(remainder, remainder, denominator);
                bigint_set_bit(quotient, shift);
            }
            bigint_shift_right(denominator, denominator, 1);
        }

        bigint_free(denominator);
    }

    quotient->neg = src_numerator_neg ^ src_denominator_neg;
    remainder->neg = src_numerator_neg;

    return dst_quotient;
}

bigint *bigint_div(
    bigint *dst,
    const bigint *numerator,
    const bigint *denominator) {
    bigint unused[1];
    bigint_init(unused);

    bigint_div_mod(dst, unused, numerator, denominator);

    bigint_free(unused);
    return dst;
}

bigint *bigint_mod(
    bigint *dst,
    const bigint *numerator,
    const bigint *denominator) {
    bigint unused[1];
    bigint_init(unused);

    bigint_div_mod(unused, dst, numerator, denominator);

    bigint_free(unused);
    return dst;
}

bigint *bigint_div_mod_half_word(
    bigint *dst,
    bigint_word *dst_remainder,
    bigint_word denominator) {
    int i, j;
    bigint_word parts[2], div_word, mod_word, remainder = 0;

    BIGINT_ASSERT(denominator, !=, 0);
    BIGINT_ASSERT(denominator, <=, BIGINT_HALF_WORD_MAX);

    for (i = dst->size - 1; i >= 0; i--) {
        bigint_word dst_word = 0;
        bigint_word src_word = dst->words[i];
        parts[1] = BIGINT_WORD_LO(src_word);
        parts[0] = BIGINT_WORD_HI(src_word);

        /* divide by denominator twice, keeping remainder in mind */
        for (j = 0; j < 2; j++) {
            remainder <<= BIGINT_WORD_BITS / 2;
            remainder |= parts[j];

            div_word = remainder / denominator;
            mod_word = remainder % denominator;
            remainder = mod_word;

            dst_word <<= BIGINT_WORD_BITS / 2;
            dst_word |= div_word;
        }

        dst->words[i] = dst_word;
    }

    *dst_remainder = remainder;
    dst->size = bigint_raw_truncate(dst->words, dst->size);
    return dst;
}

bigint *bigint_gcd(bigint *dst, const bigint *src_a, const bigint *src_b) {
    int shift, shift_a, shift_b;
    bigint a[1], b[1];

    if (src_a->size == 0) return bigint_set_neg(bigint_cpy(dst, src_b), 0);
    if (src_b->size == 0) return bigint_set_neg(bigint_cpy(dst, src_a), 0);

    if (src_a->size == 1 && src_b->size == 1) {
        bigint_word word = bigint_word_gcd(src_a->words[0], src_b->words[0]);
        return bigint_from_word(dst, word);
    }

    bigint_init(a);
    bigint_init(b);

    shift_a = bigint_count_trailing_zeros(src_a);
    shift_b = bigint_count_trailing_zeros(src_b);
    shift = BIGINT_MIN(shift_a, shift_b);

    bigint_shift_right(a, src_a, shift_a);
    bigint_shift_right(b, src_b, shift);
    a->neg = 0;
    b->neg = 0;

    do {
        bigint_shift_right(b, b, bigint_count_trailing_zeros(b));

        if (bigint_cmp_abs(a, b) > 0) BIGINT_SWAP(bigint, *a, *b);

        bigint_sub(b, b, a);
    } while (b->size != 0);

    bigint_shift_left(dst, a, shift);

    bigint_free(a);
    bigint_free(b);
    return dst;
}

bigint *bigint_sqrt(bigint *dst, const bigint *src) {
    int bit;
    bigint sum[1], tmp[1];
    const double MAX_INT_THAT_FITS_IN_DOUBLE = pow(2.0, 52.0);

    dst->neg = 0;
    dst->size = 0;

    if (src->size == 0) return dst;

    if (src->size == 1 && src->words[0] < MAX_INT_THAT_FITS_IN_DOUBLE) {
        bigint_from_word(dst, sqrt(src->words[0]));
        return dst;
    }

    bigint_init(sum);
    bigint_init(tmp);

    bigint_cpy(tmp, src);
    tmp->neg = 0;

    /* index of highest 1 bit rounded down */
    bit = bigint_bitlength(src);
    if (bit & 1) bit ^= 1;

    for (; bit >= 0; bit -= 2) {
        bigint_cpy(sum, dst);
        bigint_set_bit(sum, bit);

        if (bigint_cmp_abs(tmp, sum) >= 0) {
            bigint_sub(tmp, tmp, sum);
            bigint_set_bit(dst, bit + 1);
        }

        bigint_shift_right(dst, dst, 1);
    }

    bigint_free(tmp);
    bigint_free(sum);
    return dst;
}

char *bigint_write_base(
    char *dst,
    int *n_dst,
    const bigint *a,
    bigint_word base,
    int zero_terminate) {
    int i = 0, n = *n_dst;
    static const char *table = "0123456789abcdefghijklmnopqrstuvwxyz";
    BIGINT_ASSERT(base, >=, 2);
    BIGINT_ASSERT(base, <=, 36);

    if (zero_terminate)
        if (i < n) dst[i++] = '\0';

    if (a->size == 0) {
        if (i < n) dst[i++] = '0';
    } else {
        bigint tmp[1];
        bigint_init(tmp);
        bigint_cpy(tmp, a);

        while (tmp->size > 0) {
            bigint_word remainder;
            /* TODO extract as many digits as fit into bigint_word at once */
            /* tricky with leading zeros */
            bigint_div_mod_half_word(tmp, &remainder, base);
            if (i < n) dst[i++] = table[remainder];
        }

        bigint_free(tmp);
    }

    if (a->neg)
        if (i < n) dst[i++] = '-';
    BIGINT_REVERSE(char, dst, i);
    *n_dst = i;

    return dst;
}

char *bigint_write(char *dst, int n_dst, const bigint *a) {
    return bigint_write_base(dst, &n_dst, a, 10, 1);
}

bigint *bigint_rand_bits(bigint *dst, int n_bits, bigint_rand_func rand_func) {
    int n_word_bits = n_bits % BIGINT_WORD_BITS;
    int n_words = n_bits / BIGINT_WORD_BITS + (n_word_bits != 0);

    bigint_reserve(dst, n_words);

    rand_func((uint8_t *)dst->words, sizeof(*dst->words) * n_words);

    if (n_word_bits) {
        dst->words[n_words - 1] >>= BIGINT_WORD_BITS - n_word_bits;
    }

    dst->size = bigint_raw_truncate(dst->words, n_words);
    return dst;
}

bigint *bigint_rand_inclusive(
    bigint *dst,
    const bigint *n,
    bigint_rand_func rand_func) {
    int n_bits = bigint_bitlength(n);

    do {
        bigint_rand_bits(dst, n_bits, rand_func);
    } while (bigint_cmp(dst, n) > 0);

    return dst;
}

bigint *bigint_rand_exclusive(
    bigint *dst,
    const bigint *n,
    bigint_rand_func rand_func) {
    int n_bits = bigint_bitlength(n);

    do {
        bigint_rand_bits(dst, n_bits, rand_func);
    } while (bigint_cmp(dst, n) >= 0);

    return dst;
}

bigint *bigint_pow_mod(
    bigint *dst,
    const bigint *src_base,
    const bigint *src_exponent,
    const bigint *src_modulus) {
    bigint base[1], exponent[1], tmp[1], unused[1], modulus[1];

    bigint_init(base);
    bigint_init(exponent);
    bigint_init(tmp);
    bigint_init(unused);
    bigint_init(modulus);

    bigint_cpy(exponent, src_exponent);
    bigint_cpy(modulus, src_modulus);
    bigint_div_mod(unused, base, src_base, modulus);
    bigint_from_word(dst, 1);

    for (; exponent->size; bigint_shift_right(exponent, exponent, 1)) {
        if (bigint_get_bit(exponent, 0)) {
            bigint_mul(tmp, dst, base);
            bigint_div_mod(unused, dst, tmp, modulus);
        }
        bigint_mul(tmp, base, base);
        bigint_div_mod(unused, base, tmp, modulus);
    }

    bigint_free(base);
    bigint_free(exponent);
    bigint_free(tmp);
    bigint_free(unused);
    bigint_free(modulus);
    return dst;
}

bigint *bigint_pow_word(bigint *dst, const bigint *base, bigint_word exponent) {
    bigint result[1], p[1];

    bigint_init(p);
    bigint_init(result);

    bigint_cpy(p, base);
    bigint_from_word(result, 1);

    for (; exponent; exponent >>= 1) {
        if (exponent & 1) {
            bigint_mul(result, result, p);
            exponent--;
        }
        bigint_mul(p, p, p);
    }

    bigint_cpy(dst, result);
    bigint_free(p);
    bigint_free(result);
    return dst;
}

void bigint_raw_get_high_bits(
    bigint_word *dst, int n_dst,
    const bigint_word *src_a, int na,
    int n_bits,
    int *n_bitlength) {
    int shift;
    *n_bitlength = bigint_raw_bitlength(src_a, na);
    shift = n_bits - *n_bitlength + 1;

    if (shift < 0)
        bigint_raw_shift_right(dst, n_dst, src_a, na, -shift);
    else
        bigint_raw_shift_left(dst, n_dst, src_a, na, +shift);
}

double bigint_double(const bigint *src) {
    /* assumes IEEE 754 floating point standard */
    int n, n_mant_bits = 52;
    uint64_t x = 0, exponent = 1023;
    double d;
    bigint_word tmp[20] = { 0 };

    if (src->size == 0) return 0.0;

    bigint_raw_get_high_bits(tmp, 20, src->words, src->size, n_mant_bits, &n);
    /* this bit is stored implicitely */
    bigint_raw_clr_bit(tmp, n_mant_bits);
    exponent--;
    exponent += n;
    memcpy(&x, tmp, sizeof(x)); // NOLINT
    x |= exponent << n_mant_bits;

    memcpy(&d, &x, sizeof(d)); // NOLINT
    return src->neg ? -d : d;
}

void bigint_raw_bitwise_and(bigint *dst, const bigint_word *src_a, int na, const bigint_word *src_b, int nb) {
    int i, n = BIGINT_MIN(na, nb);
    int max = BIGINT_MAX(na, nb);
    bigint_reserve(dst, max);

    for (i = 0; i < n; i++) {
        dst->words[i] = src_a[i] & src_b[i];
    }

    for (; i < na; i++) {
        dst->words[i] = 0;
    }

    for (; i < nb; i++) {
        dst->words[i] = 0;
    }

    dst->size = max;
}

void bigint_raw_bitwise_or(bigint *dst, const bigint_word *src_a, int na, const bigint_word *src_b, int nb) {
    int i, n = BIGINT_MIN(na, nb);
    int max = BIGINT_MAX(na, nb);
    bigint_reserve(dst, max);

    for (i = 0; i < n; i++) {
        dst->words[i] = src_a[i] | src_b[i];
    }

    for (; i < na; i++) {
        dst->words[i] = src_a[i];
    }

    for (; i < nb; i++) {
        dst->words[i] = src_b[i];
    }

    dst->size = max;
    dst->size = bigint_raw_truncate(dst->words, dst->size);
}

void bigint_raw_bitwise_xor(bigint *dst, const bigint_word *src_a, int na, const bigint_word *src_b, int nb) {
    int i, n = BIGINT_MIN(na, nb);
    int max = BIGINT_MAX(na, nb);
    bigint_reserve(dst, max);

    for (i = 0; i < n; i++) {
        dst->words[i] = src_a[i] ^ src_b[i];
    }

    for (; i < na; i++) {
        int bits = bigint_word_bitlength(src_a[i]);
        dst->words[i] = (1ULL << bits) - 1;
    }

    for (; i < nb; i++) {
        int bits = bigint_word_bitlength(src_b[i]);
        dst->words[i] = (1ULL << bits) - 1;
    }

    dst->size = max;
    dst->size = bigint_raw_truncate(dst->words, dst->size);
}

void bigint_raw_bitwise_not(bigint *dst, const bigint *src, int n) {
    for (int i = 0; i < n; i++) {
        dst->words[i] = ~src->words[i];
    }
    int neg_was = src->neg;
    dst->neg = 0;
    bigint_add_word(dst, dst, 1);
    dst->neg = !neg_was;
}

void bigint_extend(bigint *dst, int n) {
    bigint_reserve(dst, n);
    bigint_raw_zero(dst->words, dst->size, n);
    dst->size = n;
}

void bigint_twos_complement_sign_extend(bigint *dst) {
    bigint_word highest_word = dst->words[dst->size - 1];
    int bits = bigint_word_bitlength(highest_word);
    bigint_word pad = ((1ULL << (BIGINT_WORD_BITS - bits)) - 1) << bits;
    dst->words[dst->size - 1] = highest_word | pad;
}

int bigint_convert_to_twos_complement(bigint *dst, int n) {
    if (!dst->neg) return dst->size;

    bigint_reserve(dst, n);
    bigint_raw_zero(dst->words, dst->size, n);
    dst->size = n;
    bigint_raw_bitwise_not(dst, dst, n);
    dst->size = bigint_raw_truncate(dst->words, dst->size);
    return dst->size;
}

void bigint_convert_from_twos_complement(bigint *dst) {
    dst->neg = 0;
    bigint_word highest_word = dst->words[dst->size - 1];
    bigint_word sign_bit = 1u << (BIGINT_WORD_BITS - 1);
    if (highest_word & sign_bit)
        bigint_raw_bitwise_not(dst, dst, dst->size);
    dst->size = bigint_raw_truncate(dst->words, dst->size);
}

int prepare_bitwise_operand(bigint *dst, const bigint *src, int n) {
    bigint_init(dst);
    bigint_cpy(dst, src);
    bigint_extend(dst, n);
    return bigint_convert_to_twos_complement(dst, n);
}

#define BIGINT_DEFINE_BITWISE_OP_FUNCTION(name, op)                        \
    bigint *name(bigint *dst, const bigint *a, const bigint *b) {          \
        int max = BIGINT_MAX(a->size, b->size);                            \
        bigint_reserve(dst, max);                                          \
        if (!a->neg && !b->neg) {                                          \
            op(dst, a->words, a->size, b->words, b->size);                 \
            dst->size = bigint_raw_truncate(dst->words, dst->size);        \
            dst->neg = 0;                                                  \
        } else {                                                           \
            bigint tmp_a[1], tmp_b[1];                                     \
            int na = prepare_bitwise_operand(tmp_a, a, max + 1);           \
            int nb = prepare_bitwise_operand(tmp_b, b, max + 1);           \
            int new_max = BIGINT_MAX(na, nb);                              \
            bigint_extend(tmp_a, new_max);                                 \
            bigint_extend(tmp_b, new_max);                                 \
            op(dst, tmp_a->words, tmp_a->size, tmp_b->words, tmp_b->size); \
            bigint_convert_from_twos_complement(dst);                      \
            bigint_free(tmp_a);                                            \
            bigint_free(tmp_b);                                            \
        }                                                                  \
        return dst;                                                        \
    }

BIGINT_DEFINE_BITWISE_OP_FUNCTION(bigint_bitwise_and, bigint_raw_bitwise_and);
BIGINT_DEFINE_BITWISE_OP_FUNCTION(bigint_bitwise_or, bigint_raw_bitwise_or);
BIGINT_DEFINE_BITWISE_OP_FUNCTION(bigint_bitwise_xor, bigint_raw_bitwise_xor);
