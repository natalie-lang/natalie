#include <stdbool.h>
#include <stddef.h>

bool test_bool(bool arg) {
    return arg;
}

unsigned char test_char(unsigned char arg) {
    return arg;
}

const char *test_char_pointer(const char *arg) {
    return arg;
}

const void *get_null() {
    return NULL;
}

size_t test_size_t(size_t arg) {
    return arg;
}

typedef enum {
    ENUMTEST_VALUE_A,
    ENUMTEST_VALUE_B,
    ENUMTEST_VALUE_C = 10,
    ENUMTEST_VALUE_D,
    ENUMTEST_VALUE_ERR = -1
} test_enum_t;

test_enum_t test_enum_call(unsigned const char c) {
    if (c == 'a') {
        return ENUMTEST_VALUE_A;
    } else if (c == 'b') {
        return ENUMTEST_VALUE_B;
    } else if (c == 'c') {
        return ENUMTEST_VALUE_C;
    } else if (c == 'd') {
        return ENUMTEST_VALUE_D;
    } else if (c == 'e') {
        return 20;
    } else {
        return ENUMTEST_VALUE_ERR;
    }
}
