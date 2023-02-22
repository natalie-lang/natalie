#define NAT_UNREACHABLE()                                                     \
    {                                                                         \
        fprintf(stderr, "panic: unreachable in %s#%d\n", __FILE__, __LINE__); \
        abort();                                                              \
    }

#define NAT_NOT_YET_IMPLEMENTED(msg, ...)                                                         \
    {                                                                                             \
        fprintf(stderr, "NOT YET IMPLEMENTED in %s#%d: " msg, __FILE__, __LINE__, ##__VA_ARGS__); \
        fprintf(stderr, "\n");                                                                    \
        abort();                                                                                  \
    }

#define NAT_RUN_BLOCK_GENERIC(env, the_block, args, block, on_break_flag) ({ \
    Natalie::Value _result = nullptr;                                        \
    do {                                                                     \
        if (_result)                                                         \
            _result->remove_redo_flag();                                     \
        _result = the_block->_run(env, args, block);                         \
        if (_result->has_break_flag()) {                                     \
            _result->remove_break_flag();                                    \
            on_break_flag;                                                   \
        }                                                                    \
    } while (_result->has_redo_flag());                                      \
    _result;                                                                 \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, args, block) ({ \
    NAT_RUN_BLOCK_GENERIC(env, the_block, args, block, return _result);  \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, args, block) ({                                                               \
    NAT_RUN_BLOCK_GENERIC(env, the_block, args, block, env->raise_local_jump_error(_result, Natalie::LocalJumpErrorType::Break)); \
})

#define NAT_RUN_BLOCK_FROM_ENV(env, args) ({                               \
    NAT_RUN_BLOCK_WITHOUT_BREAK(env, env->nearest_block(), args, nullptr); \
})

#define NAT_QUOTE(val) #val

#ifdef __APPLE__
#define NAT_ASM_PREFIX "_"
#else
#define NAT_ASM_PREFIX ""
#endif

#define NAT_MAKE_NONCOPYABLE(c) \
    c(const c &) = delete;      \
    c &operator=(const c &) = delete

#ifdef NAT_GC_GUARD
#define NAT_GC_GUARD_VALUE(val)                                                               \
    {                                                                                         \
        Object *ptr;                                                                          \
        if ((ptr = val.object_or_null()) && Heap::the().gc_enabled()) {                       \
            void *dummy;                                                                      \
            auto end_of_stack = (uintptr_t)(&dummy);                                          \
            auto start_of_stack = (uintptr_t)Heap::the().start_of_stack();                    \
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
