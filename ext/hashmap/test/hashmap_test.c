/*
 * Copyright (c) 2016-2018 David Leeds <davidesleeds@gmail.com>
 *
 * Hashmap is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <hashmap.h>

#define ARRAY_LEN(array)    (sizeof(array) / sizeof(array[0]))

#define TEST_NUM_KEYS        196607    /* Results in max load factor */
#define TEST_KEY_STR_LEN    32

void **keys_str_random;
void **keys_str_sequential;
void **keys_int_random;
void **keys_int_sequential;

struct hashmap str_map;
struct hashmap int_map;

struct test {
    const char *name;
    const char *description;
    bool (*run)(struct hashmap *map, void **keys);
    bool pre_load;
};

/*
 * Test type-specific generation macros
 */
HASHMAP_FUNCS_DECLARE(test, const void, void)
HASHMAP_FUNCS_CREATE(test, const void, void)


uint64_t test_time_us(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        assert(0);
    }
    return ((uint64_t)now.tv_sec) * 1000000 + (uint64_t)(now.tv_nsec / 1000);
}

void **test_keys_alloc(size_t num)
{
    void **keys;

    keys = (void **)calloc(num, sizeof(void *));
    if (!keys) {
        printf("malloc failed\n");
        exit(1);
    }
    return keys;
}

void *test_key_alloc_random_str(void)
{
    size_t i;
    unsigned num;
    char *key;

    key = (char *)malloc(TEST_KEY_STR_LEN + 1);
    if (!key) {
        printf("malloc failed\n");
        exit(1);
    }
    for (i = 0; i < TEST_KEY_STR_LEN; ++i) {
        num = random();
        num = (num % 96) + 32;    /* ASCII printable only */
        key[i] = (char)num;
    }
    key[TEST_KEY_STR_LEN] = '\0';
    return key;
}
void *test_key_alloc_random_int(void)
{
    uint64_t *key;

    key = (uint64_t *)malloc(sizeof(*key));
    if (!key) {
        printf("malloc failed\n");
        exit(1);
    }
    /* RAND_MAX is not guaranteed to be more than 32K */
    *key = (uint64_t)(random() & 0xffff) << 48 |
            (uint64_t)(random() & 0xffff) << 32 |
            (uint64_t)(random() & 0xffff) << 16 |
            (uint64_t)(random() & 0xffff);
    return key;
}

void *test_key_alloc_sequential_str(size_t index)
{
    char *key;

    key = (char *)malloc(TEST_KEY_STR_LEN + 1);
    if (!key) {
        printf("malloc failed\n");
        exit(1);
    }
    snprintf(key, TEST_KEY_STR_LEN + 1, "sequential key! %010zu", index);
    return key;
}

void *test_key_alloc_sequential_int(size_t index)
{
    uint64_t *key;

    key = (uint64_t *)malloc(sizeof(*key));
    if (!key) {
        printf("malloc failed\n");
        exit(1);
    }
    *key = index;
    return key;
}

void test_keys_generate(void)
{
    size_t i;

    srandom(99);    /* Use reproducible random sequences */

    keys_str_random =  test_keys_alloc(TEST_NUM_KEYS + 1);
    keys_str_sequential =  test_keys_alloc(TEST_NUM_KEYS + 1);
    keys_int_random =  test_keys_alloc(TEST_NUM_KEYS + 1);
    keys_int_sequential =  test_keys_alloc(TEST_NUM_KEYS + 1);
    for (i = 0; i < TEST_NUM_KEYS; ++i) {
        keys_str_random[i] = test_key_alloc_random_str();
        keys_str_sequential[i] = test_key_alloc_sequential_str(i);
        keys_int_random[i] = test_key_alloc_random_int();
        keys_int_sequential[i] = test_key_alloc_sequential_int(i);
    }
    keys_str_random[i] = NULL;
    keys_str_sequential[i] = NULL;
    keys_int_random[i] = NULL;
    keys_int_sequential[i] = NULL;
}

void test_load_keys(struct hashmap *map, void **keys)
{
    void **key;

    for (key = keys; *key; ++key) {
        if (!test_hashmap_put(map, *key, *key)) {
            printf("hashmap_put() failed");
            exit(1);
        }
    }
}

void test_reset_map(struct hashmap *map)
{
    hashmap_reset(map);
}

void test_print_stats(struct hashmap *map, const char *label)
{
    printf("Hashmap stats: %s\n", label);
    printf("    # entries:           %zu\n", hashmap_size(map));
    printf("    Table size:          %zu\n", map->table_size);
    printf("    Load factor:         %.4f\n", hashmap_load_factor(map));
    printf("    Collisions mean:     %.4f\n", hashmap_collisions_mean(map));
    printf("    Collisions variance: %.4f\n",
            hashmap_collisions_variance(map));

}

bool test_run(struct hashmap *map, void **keys, const struct test *t)
{
    bool success;
    uint64_t time_us;

    assert(t != NULL);
    assert(t->name != NULL);
    assert(t->run != NULL);

    if (t->pre_load) {
        printf("Pre-loading keys...");
        test_load_keys(map, keys);
        printf("done\n");
    }
    printf("Running...\n");
    time_us = test_time_us();
    success = t->run(map, keys);
    time_us = test_time_us() - time_us;
    if (success) {
        printf("Completed successfully\n");
    } else {
        printf("Failed\n");
    }
    printf("Run time: %llu microseconds\n", (long long unsigned)time_us);
    test_print_stats(map, t->name);
    test_reset_map(map);
    return success;
}

bool test_run_all(struct hashmap *map, void **keys,
        const struct test *tests, size_t num_tests, const char *env)
{
    const struct test *t;
    size_t num_failed = 0;

    printf("\n**************************************************\n");
    printf("Starting test series:\n");
    printf("    %s\n", env);
    printf("**************************************************\n\n");
    for (t = tests; t < &tests[num_tests]; ++t) {
        printf("\n**************************************************"
                "\n");
        printf("Test %02u: %s\n", (unsigned)(t - tests) + 1, t->name);
        if (t->description) {
            printf("    Description: %s\n", t->description);
        }
        printf("\n");
        if (!test_run(map, keys, t)) {
            ++num_failed;
        }
    }
    printf("\n**************************************************\n");
    printf("Test results:\n");
    printf("    Passed: %zu\n", num_tests - num_failed);
    printf("    Failed: %zu\n", num_failed);
    printf("**************************************************\n");
    return (num_failed == 0);
}

size_t test_hash_uint64(const void *key)
{
    const uint8_t *byte = (const uint8_t *)key;
    uint8_t i;
    size_t hash = 0;

    for (i = 0; i < sizeof(uint64_t); ++i, ++byte) {
        hash += *byte;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

int test_compare_uint64(const void *a, const void *b)
{
    return *(int64_t *)a - *(int64_t *)b;
}

bool test_put(struct hashmap *map, void **keys)
{
    void **key;
    void *data;

    for (key = keys; *key; ++key) {
        data = test_hashmap_put(map, *key, *key);
        if (!data) {
            printf("malloc failed\n");
            exit(1);
        }
        if (data != *key) {
            printf("duplicate key found\n");
            return false;
        }
    }
    return true;
}

bool test_put_existing(struct hashmap *map, void **keys)
{
    void **key;
    void *data;
    int temp_data = 99;

    for (key = keys; *key; ++key) {
        data = hashmap_put(map, *key, &temp_data);
        if (!data) {
            printf("malloc failed\n");
            exit(1);
        }
        if (data != *key) {
            printf("did not return existing data\n");
            return false;
        }
    }
    return true;
}

bool test_get(struct hashmap *map, void **keys)
{
    void **key;
    void *data;

    for (key = keys; *key; ++key) {
        data = test_hashmap_get(map, *key);
        if (!data) {
            printf("entry not found\n");
            return false;
        }
        if (data != *key) {
            printf("got wrong entry\n");
            return false;
        }
    }
    return true;
}

bool test_get_nonexisting(struct hashmap *map, void **keys)
{
    void **key;
    void *data;
    const char *fake_key = "test_get_nonexisting fake key!";

    for (key = keys; *key; ++key) {
        data = hashmap_get(map, fake_key);
        if (data) {
            printf("unexpected entry found\n");
            return false;
        }
    }
    return true;
}

bool test_remove(struct hashmap *map, void **keys)
{
    void **key;
    void *data;

    for (key = keys; *key; ++key) {
        data = test_hashmap_remove(map, *key);
        if (!data) {
            printf("entry not found\n");
            return false;
        }
        if (data != *key) {
            printf("removed wrong entry\n");
            return false;
        }
    }
    return true;
}

bool test_put_remove(struct hashmap *map, void **keys)
{
    size_t i = 0;
    void **key;
    void *data;

    if (!test_put(map, keys)) {
        return false;
    }
    for (key = keys; *key; ++key) {
        if (i++ >= TEST_NUM_KEYS / 2) {
            break;
        }
        data = test_hashmap_remove(map, *key);
        if (!data) {
            printf("key not found\n");
            return false;
        }
        if (data != *key) {
            printf("removed wrong entry\n");
            return false;
        }
    }
    test_print_stats(map, "test_put_remove done");
    i = 0;
    for (key = keys; *key; ++key) {
        if (i++ >= TEST_NUM_KEYS / 2) {
            break;
        }
        data = test_hashmap_put(map, *key, *key);
        if (!data) {
            printf("malloc failed\n");
            exit(1);
        }
        if (data != *key) {
            printf("duplicate key found\n");
            return false;
        }
    }
    return true;
}

bool test_iterate(struct hashmap *map, void **keys)
{
    size_t i = 0;
    struct hashmap_iter *iter = hashmap_iter(map);

    for (; iter; iter = hashmap_iter_next(map, iter)) {
        ++i;
    }
    if (i != TEST_NUM_KEYS) {
        printf("did not iterate through all entries: "
                "observed %zu, expected %u\n", i, TEST_NUM_KEYS);
        return false;
    }
    return true;
}

bool test_iterate_remove(struct hashmap *map, void **keys)
{
    size_t i = 0;
    struct hashmap_iter *iter = hashmap_iter(map);
    const void *key;

    while (iter) {
        ++i;
        key = test_hashmap_iter_get_key(iter);
        if (test_hashmap_get(map, key) != key) {
            printf("invalid iterator on entry #%zu\n", i);
            return false;
        }
        iter = hashmap_iter_remove(map, iter);
        if (test_hashmap_get(map, key) != NULL) {
            printf("iter_remove failed on entry #%zu\n", i);
            return false;
        }
    }
    if (i != TEST_NUM_KEYS) {
        printf("did not iterate through all entries: "
                "observed %zu, expected %u\n", i, TEST_NUM_KEYS);
        return false;
    }
    return true;
}

struct test_foreach_arg {
    struct hashmap *map;
    size_t i;
};

int test_foreach_callback(const void *key, void *data, void *arg)
{
    struct test_foreach_arg *state = (struct test_foreach_arg *)arg;

    if (state->i & 1) {
        /* Remove every other key */
        if (!test_hashmap_remove(state->map, key)) {
            printf("could not remove expected key\n");
            return -1;
        }
    }
    ++state->i;
    return 0;
}

bool test_foreach(struct hashmap *map, void **keys)
{
    struct test_foreach_arg arg = { map, 1 };
    size_t size = hashmap_size(map);

    if (test_hashmap_foreach(map, test_foreach_callback, &arg) < 0) {
        return false;
    }
    if (hashmap_size(map) != size / 2) {
        printf("foreach delete did not remove expected # of entries: "
                "contains %zu vs. expected %zu\n", hashmap_size(map),
                size / 2);
        return false;
    }
    return true;
}

bool test_clear(struct hashmap *map, void **keys)
{
    hashmap_clear(map);
    return true;
}

bool test_reset(struct hashmap *map, void **keys)
{
    hashmap_reset(map);
    return true;
}

const struct test tests[] = {
        {
                .name = "put performance",
                .description = "put new hash keys",
                .run = test_put
        },
        {
                .name = "put existing performance",
                .description = "attempt to put existing hash keys",
                .run = test_put_existing,
                .pre_load = true
        },
        {
                .name = "get existing performance",
                .description = "get existing hash keys",
                .run = test_get,
                .pre_load = true
        },
        {
                .name = "get non-existing performance",
                .description = "get nonexistent hash keys",
                .run = test_get_nonexisting,
                .pre_load = true
        },
        {
                .name = "remove performance",
                .description = "remove hash keys",
                .run = test_remove,
                .pre_load = true
        },
        {
                .name = "mixed put/remove performance",
                .description = "put, remove 1/2, then put them back",
                .run = test_put_remove
        },
        {
                .name = "iterate performance",
                .description = "iterate through entries",
                .run = test_iterate,
                .pre_load = true
        },
        {
                .name = "iterate remove all",
                .description = "iterate and remove all entries",
                .run = test_iterate_remove,
                .pre_load = true
        },
        {
                .name = "removal in foreach",
                .description = "iterate and delete 1/2 using hashmap_foreach",
                .run = test_foreach,
                .pre_load = true
        },
        {
                .name = "clear performance",
                .description = "clear entries",
                .run = test_clear,
                .pre_load = true
        },
        {
                .name = "reset performance",
                .description = "reset entries",
                .run = test_reset,
                .pre_load = true
        }
};

/*
 * Main function
 */
int main(int argc, char **argv)
{
    bool success = true;

    /* Initialize */
    printf("Initializing hash maps...");
    if (hashmap_init(&str_map, hashmap_hash_string, hashmap_compare_string,
            0) < 0) {
        success = false;
    }
    /*
    hashmap_set_key_alloc_funcs(&str_map, hashmap_alloc_key_string, free);
     */
    if (hashmap_init(&int_map, test_hash_uint64, test_compare_uint64,
            0) < 0) {
        success = false;
    }
    printf("done\n");

    if (!success) {
        printf("Hashmap init failed");
        return 1;
    }

    printf("Generating test %u test keys...", TEST_NUM_KEYS);
    test_keys_generate();
    printf("done\n");

    printf("Running tests\n\n");
    success &= test_run_all(&str_map, keys_str_random, tests,
            ARRAY_LEN(tests), "Hashmap w/randomized string keys");
    success &= test_run_all(&str_map, keys_str_sequential, tests,
            ARRAY_LEN(tests), "Hashmap w/sequential string keys");

    success &= test_run_all(&int_map, keys_int_random, tests,
            ARRAY_LEN(tests), "Hashmap w/randomized integer keys");

    success &= test_run_all(&int_map, keys_int_sequential, tests,
            ARRAY_LEN(tests), "Hashmap w/sequential integer keys");

    printf("\nTests finished\n");

    hashmap_destroy(&str_map);
    hashmap_destroy(&int_map);

    if (!success) {
        printf("Tests FAILED\n");
        exit(1);
    }
    return 0;
}
