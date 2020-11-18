#define NAT_UNREACHABLE()                        \
    {                                            \
        fprintf(stderr, "panic: unreachable\n"); \
        abort();                                 \
    }

#define NAT_NOT_YET_IMPLEMENTED(description)                     \
    {                                                            \
        fprintf(stderr, "NOT YET IMPLEMENTED: %s", description); \
        abort();                                                 \
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
    Value *_result = the_block->_run(env, argc, args, block);                                   \
    if (_result->has_break_flag()) {                                                            \
        if (env == env_with_block) {                                                            \
            _result->remove_break_flag();                                                       \
        }                                                                                       \
        return _result;                                                                         \
    }                                                                                           \
    _result;                                                                                    \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({ \
    Value *_result = the_block->_run(env, argc, args, block);                  \
    if (_result->has_break_flag()) {                                           \
        _result->remove_break_flag();                                          \
        return _result;                                                        \
    }                                                                          \
    _result;                                                                   \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({ \
    Natalie::Value *_result = the_block->_run(env, argc, args, block);    \
    if (_result->has_break_flag()) {                                      \
        _result->remove_break_flag();                                     \
        env->raise_local_jump_error(_result, "break from proc-closure");  \
    }                                                                     \
    _result;                                                              \
})

#define NAT_QUOTE(val) #val

#ifdef __APPLE__
#define NAT_ASM_PREFIX "_"
#else
#define NAT_ASM_PREFIX ""
#endif
