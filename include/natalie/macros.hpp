#include <sys/time.h>

#define NAT_UNREACHABLE()                                                     \
    {                                                                         \
        fprintf(stderr, "panic: unreachable in %s#%d\n", __FILE__, __LINE__); \
        abort();                                                              \
    }

#define NAT_NOT_YET_IMPLEMENTED(msg, ...)                                                              \
    {                                                                                                  \
        fprintf(stderr, "NOT YET IMPLEMENTED in %s#%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        abort();                                                                                       \
    }

#define NAT_QUOTE(val) #val

#define NAT_MAKE_NONCOPYABLE(c) \
    c(const c &) = delete;      \
    c &operator=(const c &) = delete

#ifdef NAT_GC_GUARD
#ifdef __SANITIZE_ADDRESS__
#error "Do not enable ASan and NAT_GC_GUARD at the same time"
#endif
#define NAT_GC_GUARD_VALUE(val)                                                               \
    {                                                                                         \
        Object *ptr;                                                                          \
        if (!val.is_integer() && (ptr = val.object()) && Heap::the().gc_enabled()) {          \
            std::lock_guard<std::recursive_mutex> gc_lock(Natalie::g_gc_recursive_mutex);     \
            auto end_of_stack = (uintptr_t)__builtin_frame_address(0);                        \
            auto start_of_stack = (uintptr_t)(ThreadObject::current()->start_of_stack());     \
            assert(start_of_stack > end_of_stack);                                            \
            if ((uintptr_t)ptr > end_of_stack && (uintptr_t)ptr < start_of_stack) {           \
                fprintf(                                                                      \
                    stderr,                                                                   \
                    "This object (%p) is stack allocated, but you allowed it to be captured " \
                    "in a Ruby variable or another data structure.\n"                         \
                    "Be sure to heap-allocate the object with `new`.",                        \
                    ptr);                                                                     \
                abort();                                                                      \
            }                                                                                 \
        }                                                                                     \
    }
#else
#define NAT_GC_GUARD_VALUE(val)
#endif

#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))

#ifdef NAT_DEBUG_THREADS
#define NAT_THREAD_DEBUG(msg, ...) \
    fprintf(stderr, "THREAD DEBUG: " msg "\n", ##__VA_ARGS__)
#else
#define NAT_THREAD_DEBUG(msg, ...)
#endif

#define NAT_LOG(msg, ...)                                                               \
    {                                                                                   \
        struct timeval tv;                                                              \
        gettimeofday(&tv, nullptr);                                                     \
        fprintf(stderr, "[%ld.%06ld] " msg "\n", tv.tv_sec, tv.tv_usec, ##__VA_ARGS__); \
    }
