#pragma once

// https://github.com/983/bigint

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdint.h>

/* any unsigned integer type */
typedef uint32_t bigint_word;

#define BIGINT_KARATSUBA_WORD_THRESHOLD 20

#define BIGINT_WORD_BITS ((sizeof(bigint_word) * CHAR_BIT))
#define BIGINT_WORD_MAX ((bigint_word)-1)
#define BIGINT_HALF_WORD_MAX (BIGINT_WORD_MAX >> BIGINT_WORD_BITS / 2)

#define BIGINT_WORD_LO(a) ((a)&BIGINT_HALF_WORD_MAX)
#define BIGINT_WORD_HI(a) ((a) >> sizeof(a) * CHAR_BIT / 2)

#define BIGINT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BIGINT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define BIGINT_INT_ABS(a) ((a) < 0 ? -(unsigned int)(a) : (unsigned int)(a))

#define BIGINT_SWAP(type, a, b) \
    do {                        \
        type _tmp = a;          \
        a = b;                  \
        b = _tmp;               \
    } while (0)

#define BIGINT_REVERSE(type, data, n)                      \
    do {                                                   \
        int _i;                                            \
        for (_i = 0; _i < (n) / 2; _i++)                   \
            BIGINT_SWAP(type, data[_i], data[n - 1 - _i]); \
    } while (0)

typedef struct bigint {
    bigint_word *words;
    int neg, size, capacity;
} bigint;

typedef void (*bigint_rand_func)(uint8_t *dst, int n);

bigint_word bigint_word_mul_lo(bigint_word a, bigint_word b);
bigint_word bigint_word_mul_hi(bigint_word a, bigint_word b);

bigint_word bigint_word_add_get_carry(bigint_word *dst, bigint_word a, bigint_word b);
bigint_word bigint_word_sub_get_carry(bigint_word *dst, bigint_word a, bigint_word b);

bigint_word bigint_word_from_char(char c);

int bigint_word_bitlength(bigint_word a);
int bigint_word_count_trailing_zeros(bigint_word a);

bigint_word bigint_word_gcd(bigint_word a, bigint_word b);
unsigned bigint_uint_gcd(unsigned a, unsigned b);
int bigint_int_gcd(int a, int b);

bigint *bigint_init(bigint *a);
bigint *bigint_reserve(bigint *a, int capacity);
void bigint_free(bigint *a);

int bigint_cmp_abs(const bigint *a, const bigint *b);
int bigint_cmp(const bigint *a, const bigint *b);
int bigint_cmp_abs_word(const bigint *a, bigint_word b);

void bigint_raw_zero(bigint_word *dst, int from, int to);
int bigint_raw_truncate(const bigint_word *a, int n);

bigint *bigint_set_neg(bigint *dst, int neg);
bigint *bigint_negate(bigint *dst);

bigint *bigint_cpy(bigint *dst, const bigint *src);

bigint *bigint_clr_bit(bigint *dst, unsigned bit_index);
bigint *bigint_set_bit(bigint *dst, unsigned bit_index);
bigint_word bigint_get_bit(const bigint *src, unsigned bit_index);

bigint *bigint_mul(bigint *dst, const bigint *a, const bigint *b);

int bigint_count_digits(const char *src);
int bigint_digits_bound(int n_digits_src, double src_base, double dst_base);
int bigint_write_size(const bigint *a, double dst_base);
bigint *bigint_from_str_base(bigint *dst, const char *src, int src_base);
bigint *bigint_from_str(bigint *dst, const char *src);
bigint *bigint_from_int(bigint *dst, int src);
bigint *bigint_from_word(bigint *dst, bigint_word a);

bigint *bigint_add_signed(bigint *dst, const bigint *a, int a_neg, const bigint *b, int b_neg);
bigint *bigint_add(bigint *dst, const bigint *a, const bigint *b);
bigint *bigint_sub(bigint *dst, const bigint *a, const bigint *b);
bigint *bigint_add_word_signed(bigint *dst, const bigint *src_a, bigint_word b, int b_neg);
bigint *bigint_add_word(bigint *dst, const bigint *src_a, bigint_word b);
bigint *bigint_sub_word(bigint *dst, const bigint *src_a, bigint_word b);

char *bigint_write_base(
    char *dst,
    int *n_dst,
    const bigint *a,
    bigint_word base,
    int zero_terminate);

/* convenience function defaults to base 10 and zero terminates */
char *bigint_write(char *dst, int n_dst, const bigint *a);

bigint *bigint_shift_left(bigint *dst, const bigint *src, unsigned long long shift);
bigint *bigint_shift_right(bigint *dst, const bigint *src, unsigned long long shift);

void bigint_twos_complement_sign_extend(bigint *dst);
int bigint_convert_to_twos_complement(bigint *dst, int n);
void bigint_convert_from_twos_complement(bigint *dst);

int bigint_bitlength(const bigint *a);
int bigint_count_trailing_zeros(const bigint *a);

bigint *bigint_div_mod(
    bigint *dst_quotient,
    bigint *dst_remainder,
    const bigint *src_biginterator,
    const bigint *src_denominator);

bigint *bigint_div(
    bigint *dst,
    const bigint *numerator,
    const bigint *denominator);

bigint *bigint_mod(
    bigint *dst,
    const bigint *numerator,
    const bigint *denominator);

bigint *bigint_div_mod_half_word(
    bigint *dst,
    bigint_word *dst_remainder,
    bigint_word denominator);

bigint *bigint_gcd(bigint *dst, const bigint *src_a, const bigint *src_b);
bigint *bigint_sqrt(bigint *dst, const bigint *src);

bigint *bigint_rand_bits(bigint *dst, int n_bits, bigint_rand_func rand_func);
bigint *bigint_rand_inclusive(bigint *dst, const bigint *n, bigint_rand_func rand_func);
bigint *bigint_rand_exclusive(bigint *dst, const bigint *n, bigint_rand_func rand_func);

bigint *bigint_pow_mod(
    bigint *dst,
    const bigint *src_base,
    const bigint *src_exponent,
    const bigint *src_modulus);

bigint *bigint_pow_word(bigint *dst, const bigint *src, bigint_word exponent);

double bigint_double(const bigint *src);

bigint *bigint_bitwise_and(bigint *dst, const bigint *a, const bigint *b);
bigint *bigint_bitwise_or(bigint *dst, const bigint *a, const bigint *b);
bigint *bigint_bitwise_xor(bigint *dst, const bigint *a, const bigint *b);
bigint *bigint_bitwise_not(bigint *dst, const bigint *src);

#ifdef __cplusplus
}
#endif
