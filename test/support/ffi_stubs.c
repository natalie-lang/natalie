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

int test_int(int arg) {
    return arg;
}

size_t test_size_t(size_t arg) {
    return arg;
}

const char *test_string() {
    return "string";
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

char test_enum_argument(const test_enum_t e) {
    switch (e) {
    case ENUMTEST_VALUE_A:
        return 'a';
    case ENUMTEST_VALUE_B:
        return 'b';
    case ENUMTEST_VALUE_C:
        return 'c';
    case ENUMTEST_VALUE_D:
        return 'd';
    case ENUMTEST_VALUE_ERR:
        return 'e';
    default:
        return 'x';
    }
}
