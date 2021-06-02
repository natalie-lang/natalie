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

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                                        \
    Env *env_with_block = env;                                                                            \
    while (!env_with_block->block() && env_with_block->outer()) {                                         \
        env_with_block = env_with_block->outer();                                                         \
    }                                                                                                     \
    if (!env_with_block->block()) {                                                                       \
        env->raise("LocalJumpError", "no block given");                                                   \
    }                                                                                                     \
    NAT_RUN_BLOCK_AND_PROPAGATE_BREAK(env, env_with_block, env_with_block->block(), argc, args, nullptr); \
})

#define NAT_RUN_BLOCK_AND_PROPAGATE_BREAK(env, env_with_block, the_block, argc, args, block) ({ \
    ValuePtr _result = the_block->_run(env, argc, args, block);                                 \
    if (_result->has_break_flag()) {                                                            \
        if (env == env_with_block) {                                                            \
            _result->remove_break_flag();                                                       \
        }                                                                                       \
        return _result;                                                                         \
    }                                                                                           \
    _result;                                                                                    \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({ \
    ValuePtr _result = the_block->_run(env, argc, args, block);                \
    if (_result->has_break_flag()) {                                           \
        _result->remove_break_flag();                                          \
        return _result;                                                        \
    }                                                                          \
    _result;                                                                   \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WITH_CLEANUP(env, the_block, argc, args, block, cleanup_code) ({ \
    ValuePtr _result = the_block->_run(env, argc, args, block);                                           \
    if (_result->has_break_flag()) {                                                                      \
        _result->remove_break_flag();                                                                     \
        cleanup_code;                                                                                     \
        return _result;                                                                                   \
    }                                                                                                     \
    cleanup_code;                                                                                         \
    _result;                                                                                              \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({ \
    Natalie::ValuePtr _result = the_block->_run(env, argc, args, block);  \
    if (_result->has_break_flag()) {                                      \
        _result->remove_break_flag();                                     \
        env->raise_local_jump_error(_result, "break from proc-closure");  \
    }                                                                     \
    _result;                                                              \
})

#define NAT_CALL_BEGIN(env, self, begin_fn, argc, args, block) ({     \
    Env *e = new Env { env };                                         \
    e->set_caller(env);                                               \
    Natalie::ValuePtr _result = begin_fn(e, self, argc, args, block); \
    e->clear_caller();                                                \
    if (_result->has_break_flag()) {                                  \
        return _result;                                               \
    }                                                                 \
    _result;                                                          \
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

#define NAT_MIN(a, b) (a < b ? a : b)
