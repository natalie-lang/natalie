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

#define NAT_RUN_BLOCK_GENERIC(env, the_block, argc, args, block, cleanup_code, on_break_flag) ({ \
    Natalie::ValuePtr _result = nullptr;                                                         \
    do {                                                                                         \
        if (_result)                                                                             \
            _result->remove_redo_flag();                                                         \
        _result = the_block->_run(env, argc, args, block);                                       \
        if (_result->has_break_flag()) {                                                         \
            _result->remove_break_flag();                                                        \
            cleanup_code;                                                                        \
            on_break_flag;                                                                       \
        }                                                                                        \
    } while (_result->has_redo_flag());                                                          \
    cleanup_code;                                                                                \
    _result;                                                                                     \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WITH_CLEANUP(env, the_block, argc, args, block, cleanup_code) ({ \
    NAT_RUN_BLOCK_GENERIC(env, the_block, argc, args, block, cleanup_code, return _result);               \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({    \
    NAT_RUN_BLOCK_GENERIC(env, the_block, argc, args, block, {}, return _result); \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({                                                                   \
    NAT_RUN_BLOCK_GENERIC(env, the_block, argc, args, block, {}, env->raise_local_jump_error(_result, Natalie::LocalJumpErrorType::Break)); \
})

#define NAT_HANDLE_BREAK(value, on_break_flag) ({ \
    if (value->has_break_flag()) {                \
        value->remove_break_flag();               \
        on_break_flag;                            \
    }                                             \
})

#define NAT_HANDLE_LOCAL_JUMP_ERROR(env, exception, return_handler) ({                                                           \
    auto LocalJumpError = self->const_find(env, SymbolValue::intern("LocalJumpError"), Value::ConstLookupSearchMode::NotStrict); \
    ValuePtr result;                                                                                                             \
    if (!exception->is_a(env, LocalJumpError)) {                                                                                 \
        throw exception;                                                                                                         \
    } else if (return_handler && exception->local_jump_error_type() == LocalJumpErrorType::Return) {                             \
        result = exception->_send(env, SymbolValue::intern("exit_value"));                                                       \
    } else if (env && exception->local_jump_error_env() == env) {                                                                \
        result = exception->_send(env, SymbolValue::intern("exit_value"));                                                       \
    } else {                                                                                                                     \
        throw exception;                                                                                                         \
    }                                                                                                                            \
    result;                                                                                                                      \
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

#define NAT_SEND(env, receiver, method_name, argc, args, block, method_cache, call_site_index) ({            \
    ValuePtr _result = nullptr;                                                                              \
    if (!receiver.is_pointer()) {                                                                            \
        _result = receiver.send(env, method_name, argc, args, block);                                        \
    } else {                                                                                                 \
        auto cached_method = method_cache[call_site_index];                                                  \
        if (cached_method) {                                                                                 \
            receiver->ensure_method_is_callable(env, method_name, cached_method, MethodVisibility::Private); \
            _result = cached_method->call(env, receiver, argc, args, block);                                 \
        } else {                                                                                             \
            Method *method = receiver->find_method(env, method_name, MethodVisibility::Private);             \
            method_cache[call_site_index] = method;                                                          \
            _result = method->call(env, receiver, argc, args, block);                                        \
        }                                                                                                    \
    }                                                                                                        \
    _result;                                                                                                 \
})

#define NAT_PUBLIC_SEND(env, receiver, method_name, argc, args, block, method_cache, call_site_index) ({    \
    ValuePtr _result = nullptr;                                                                             \
    if (!receiver.is_pointer()) {                                                                           \
        _result = receiver.public_send(env, method_name, argc, args, block);                                \
    } else {                                                                                                \
        auto cached_method = method_cache[call_site_index];                                                 \
        if (cached_method) {                                                                                \
            receiver->ensure_method_is_callable(env, method_name, cached_method, MethodVisibility::Public); \
            _result = cached_method->call(env, receiver, argc, args, block);                                \
        } else {                                                                                            \
            Method *method = receiver->find_method(env, method_name, MethodVisibility::Public);             \
            method_cache[call_site_index] = method;                                                         \
            _result = method->call(env, receiver, argc, args, block);                                       \
        }                                                                                                   \
    }                                                                                                       \
    _result;                                                                                                \
})
