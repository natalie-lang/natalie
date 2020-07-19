#define NAT_RAISE(env, class_name, message_format, ...)                                                      \
    {                                                                                                        \
        env->raise(NAT_OBJECT->const_get(env, class_name, true)->as_class(), message_format, ##__VA_ARGS__); \
        abort();                                                                                             \
    }

#define NAT_RAISE2(env, constant, message_format, ...)       \
    {                                                        \
        env->raise(constant, message_format, ##__VA_ARGS__); \
        abort();                                             \
    }

#define NAT_ASSERT_ARGC(...)                                       \
    NAT_GET_MACRO(__VA_ARGS__, NAT_ASSERT_ARGC2, NAT_ASSERT_ARGC1) \
    (__VA_ARGS__)

#define NAT_ASSERT_ARGC1(expected)                                                                            \
    if (argc != expected) {                                                                                   \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, expected); \
    }

#define NAT_ASSERT_ARGC2(expected_low, expected_high)                                                                                \
    if (argc < expected_low || argc > expected_high) {                                                                               \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d..%d)", argc, expected_low, expected_high); \
    }

#define NAT_ASSERT_ARGC_AT_LEAST(expected)                                                                     \
    if (argc < expected) {                                                                                     \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d+)", argc, expected); \
    }

#define NAT_ASSERT_TYPE(obj, expected_type, expected_class_name)                                                                \
    if ((obj->type()) != expected_type) {                                                                                       \
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", (obj->klass())->class_name(), expected_class_name); \
    }

#define NAT_GET_MACRO(_1, _2, NAME, ...) NAME

#define NAT_UNREACHABLE()                        \
    {                                            \
        fprintf(stderr, "panic: unreachable\n"); \
        abort();                                 \
    }

#define NAT_ASSERT_NOT_FROZEN(obj)                                                                                 \
    if (obj->is_frozen()) {                                                                                        \
        NAT_RAISE(env, "FrozenError", "can't modify frozen %s: %s", obj->klass()->class_name(), NAT_INSPECT(obj)); \
    }

#define NAT_ASSERT_BLOCK()                                         \
    if (!block) {                                                  \
        NAT_RAISE(env, "ArgumentError", "called without a block"); \
    }

#define NAT_MIN(a, b) ((a < b) ? a : b)
#define NAT_MAX(a, b) ((a > b) ? a : b)

#define NAT_NOT_YET_IMPLEMENTED(description)                     \
    {                                                            \
        fprintf(stderr, "NOT YET IMPLEMENTED: %s", description); \
        abort();                                                 \
    }

#define NAT_OBJ_HAS_ENV(obj) ((obj)->env.global_env == env->global_env) // prefered check
#define NAT_OBJ_HAS_ENV2(obj) ((obj)->env.global_env) // limited check used when there is no current env, i.e. hashmap_hash and hashmap_compare

#define NAT_INSPECT(obj) obj->send(env, "inspect")->as_string()->c_str()

// ahem, "globals"
#define NAT_OBJECT env->global_env->Object()
#define NAT_NIL env->global_env->nil()
#define NAT_TRUE env->global_env->true_obj()
#define NAT_FALSE env->global_env->false_obj()

#define NAT_MIN_INT INT64_MIN
#define NAT_MAX_INT INT64_MAX

#define NAT_MIN_FLOAT DBL_MIN
#define NAT_MAX_FLOAT DBL_MAX

#define NAT_INT_64_MAX_BUF_LEN 21 // 1 for sign, 19 for max digits, and 1 for null terminator

// "0x" + up to 16 hex chars + nullptr terminator
#define NAT_OBJECT_POINTER_BUF_LENGTH 2 + 16 + 1

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_FROZEN 2
#define NAT_FLAG_BREAK 4

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                     \
    Env *env_with_block = env;                                                         \
    while (!env_with_block->block && env_with_block->outer) {                          \
        env_with_block = env_with_block->outer;                                        \
    }                                                                                  \
    if (!env_with_block->block) {                                                      \
        NAT_RAISE(env, "LocalJumpError", "no block given");                            \
    }                                                                                  \
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, env_with_block->block, argc, args, nullptr); \
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
    Value *_result = the_block->_run(env, argc, args, block);             \
    if (_result->has_break_flag()) {                                      \
        _result->remove_break_flag();                                     \
        env->raise_local_jump_error(_result, "break from proc-closure");  \
        abort();                                                          \
    }                                                                     \
    _result;                                                              \
})

#define EMPTY_HASHMAP \
    { 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr }

#define NAT_QUOTE(val) #val

#define NAT_BRIDGE_METHOD(...) // dummy macro replaced by our compiler later
