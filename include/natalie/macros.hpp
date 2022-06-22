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

#define NAT_RUN_BLOCK_FROM_ENV(env, args) ({                                                        \
    Env *env_with_block = env;                                                                      \
    while (!env_with_block->block() && env_with_block->outer()) {                                   \
        env_with_block = env_with_block->outer();                                                   \
    }                                                                                               \
    if (!env_with_block->block()) {                                                                 \
        env->raise("LocalJumpError", "no block given");                                             \
    }                                                                                               \
    NAT_RUN_BLOCK_AND_PROPAGATE_BREAK(env, env_with_block, env_with_block->block(), args, nullptr); \
})

#define NAT_RUN_BLOCK_AND_PROPAGATE_BREAK(env, env_with_block, the_block, args, block) ({ \
    Value _result = the_block->_run(env, args, block);                                    \
    if (_result->has_break_flag()) {                                                      \
        if (env == env_with_block) {                                                      \
            _result->remove_break_flag();                                                 \
        }                                                                                 \
        return _result;                                                                   \
    }                                                                                     \
    _result;                                                                              \
})

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

#define NAT_HANDLE_BREAK(value, on_break_flag) ({ \
    if (value->has_break_flag()) {                \
        value->remove_break_flag();               \
        on_break_flag;                            \
    }                                             \
})

#define NAT_HANDLE_LOCAL_JUMP_ERROR(env, exception, return_handler) ({                               \
    auto LocalJumpError = find_top_level_const(env, "LocalJumpError"_s);                             \
    Value result;                                                                                    \
    if (!exception->is_a(env, LocalJumpError)) {                                                     \
        throw exception;                                                                             \
    } else if (return_handler && exception->local_jump_error_type() == LocalJumpErrorType::Return) { \
        result = exception->send(env, "exit_value"_s);                                               \
    } else if (env && exception->local_jump_error_env() == env) {                                    \
        result = exception->send(env, "exit_value"_s);                                               \
    } else {                                                                                         \
        throw exception;                                                                             \
    }                                                                                                \
    result;                                                                                          \
})

// if a LocalJumpError does not belong to the current scope, then bubble it up higher
#define NAT_RERAISE_LOCAL_JUMP_ERROR(env, exception) ({                                                                     \
    if (exception->local_jump_error_env()) {                                                                                \
        if (exception->local_jump_error_env() != env || exception->local_jump_error_type() == LocalJumpErrorType::Return) { \
            throw exception;                                                                                                \
        }                                                                                                                   \
    }                                                                                                                       \
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
