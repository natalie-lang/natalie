#pragma once

#define TM_UNREACHABLE()                                                      \
    {                                                                         \
        fprintf(stderr, "panic: unreachable in %s#%d\n", __FILE__, __LINE__); \
        abort();                                                              \
    }

#define TM_NOT_YET_IMPLEMENTED(msg, ...)                                                          \
    {                                                                                             \
        fprintf(stderr, "NOT YET IMPLEMENTED in %s#%d: " msg, __FILE__, __LINE__, ##__VA_ARGS__); \
        fprintf(stderr, "\n");                                                                    \
        abort();                                                                                  \
    }

#define TM_UNUSED(var) (void)var
