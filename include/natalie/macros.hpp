#define NAT_IS_TAGGED_INT(obj) ((int64_t)obj & 1)
#define NAT_TYPE(obj) (NAT_IS_TAGGED_INT(obj) ? Value::Type::Integer : obj->type)
#define NAT_IS_MODULE_OR_CLASS(obj) (NAT_TYPE(obj) == Value::Type::Module || NAT_TYPE(obj) == Value::Type::Class)
#define NAT_OBJ_CLASS(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_INTEGER : obj->klass)
#define NAT_RESCUE(env) ((env->rescue = 1) && setjmp(env->jump_buf))

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

#define NAT_ASSERT_TYPE(obj, expected_type, expected_class_name)                                                                      \
    if (NAT_TYPE((obj)) != expected_type) {                                                                                           \
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", NAT_OBJ_CLASS((obj))->class_name(), expected_class_name); \
    }

#define NAT_GET_MACRO(_1, _2, NAME, ...) NAME

#define NAT_UNREACHABLE()                        \
    {                                            \
        fprintf(stderr, "panic: unreachable\n"); \
        abort();                                 \
    }

#define NAT_ASSERT_NOT_FROZEN(obj)                                                                                       \
    if (is_frozen(obj)) {                                                                                                \
        NAT_RAISE(env, "FrozenError", "can't modify frozen %s: %S", NAT_OBJ_CLASS(obj)->class_name(), NAT_INSPECT(obj)); \
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

#define NAT_INSPECT(obj) obj->send(env, "inspect", 0, nullptr, nullptr)

// ahem, "globals"
#define NAT_OBJECT env->global_env->Object
#define NAT_INTEGER env->global_env->Integer
#define NAT_NIL env->global_env->nil
#define NAT_TRUE env->global_env->true_obj
#define NAT_FALSE env->global_env->false_obj

// FIXME: Either use Autoconf or find another way to define this
#ifdef PORTABLE_RSHIFT
#define RSHIFT(x, y) (((x) < 0) ? ~((~(x)) >> y) : (x) >> y)
#else
#define RSHIFT(x, y) ((x) >> (int)y)
#endif

// only 63-bit numbers for now
#define NAT_MIN_INT RSHIFT(INT64_MIN, 1)
#define NAT_MAX_INT RSHIFT(INT64_MAX, 1)

#define NAT_INT_64_MAX_BUF_LEN 21 // 1 for sign, 19 for max digits, and 1 for null terminator

// "0x" + up to 16 hex chars + nullptr terminator
#define NAT_OBJECT_POINTER_BUF_LENGTH 2 + 16 + 1

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_FROZEN 2
#define NAT_FLAG_BREAK 4

#define is_main_object(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT))

#define is_frozen(obj) ((obj->is_integer() || ((obj)->flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN))

#define freeze_object(obj) obj->flags = obj->flags | NAT_FLAG_FROZEN;

#define flag_break(obj) \
    obj->flags = obj->flags | NAT_FLAG_BREAK;

#define is_break(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK))

#define remove_break(obj) ((obj)->flags = (obj)->flags & ~NAT_FLAG_BREAK)

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                         \
    Env *env_with_block = env;                                                             \
    while (!env_with_block->block && env_with_block->outer) {                              \
        env_with_block = env_with_block->outer;                                            \
    }                                                                                      \
    Value *_result = _run_block_internal(env, env_with_block->block, argc, args, nullptr); \
    if (is_break(_result)) {                                                               \
        remove_break(_result);                                                             \
        return _result;                                                                    \
    }                                                                                      \
    _result;                                                                               \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({ \
    Value *_result = _run_block_internal(env, the_block, argc, args, block);   \
    if (is_break(_result)) {                                                   \
        remove_break(_result);                                                 \
        return _result;                                                        \
    }                                                                          \
    _result;                                                                   \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({    \
    Value *_result = _run_block_internal(env, the_block, argc, args, block); \
    if (is_break(_result)) {                                                 \
        remove_break(_result);                                               \
        env->raise_local_jump_error(_result, "break from proc-closure");     \
        abort();                                                             \
    }                                                                        \
    _result;                                                                 \
})

#define EMPTY_HASHMAP \
    { 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr }
