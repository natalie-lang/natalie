#include "natalie.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

namespace Natalie {

bool is_constant_name(const char *name) {
    return strlen(name) > 0 && isupper(name[0]);
}

bool is_global_name(const char *name) {
    return strlen(name) > 0 && name[0] == '$';
}

Value *splat(Env *env, Value *obj) {
    if (obj->is_array()) {
        return new ArrayValue { *obj->as_array() };
    } else {
        return to_ary(env, obj, false);
    }
}

void int_to_string(int64_t num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
    } else {
        snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "%" PRId64, num);
    }
}

void int_to_hex_string(int64_t num, char *buf, bool capitalize) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
    } else {
        if (capitalize) {
            snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "0X%" PRIX64, num);
        } else {
            snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "0x%" PRIx64, num);
        }
    }
}

Value *call_begin(Env *env, Value *self, Value *(*block_fn)(Env *, Value *)) {
    Env e = Env::new_block_env(env, env);
    return block_fn(&e, self);
}

void run_at_exit_handlers(Env *env) {
    ArrayValue *at_exit_handlers = env->global_get("$NAT_at_exit_handlers")->as_array();
    assert(at_exit_handlers);
    for (int i = at_exit_handlers->size() - 1; i >= 0; i--) {
        Value *proc = (*at_exit_handlers)[i];
        assert(proc);
        assert(proc->is_proc());
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->as_proc()->block(), 0, nullptr, nullptr);
    }
}

void print_exception_with_backtrace(Env *env, ExceptionValue *exception) {
    IoValue *stderr = env->global_get("$stderr")->as_io();
    int fd = stderr->fileno();
    const ArrayValue *backtrace = exception->backtrace();
    if (backtrace && backtrace->size() > 0) {
        dprintf(fd, "Traceback (most recent call last):\n");
        for (int i = backtrace->size() - 1; i > 0; i--) {
            StringValue *line = (*backtrace)[i]->as_string();
            assert(line->type() == Value::Type::String);
            dprintf(fd, "        %d: from %s\n", i, line->c_str());
        }
        StringValue *line = (*backtrace)[0]->as_string();
        dprintf(fd, "%s: ", line->c_str());
    }
    dprintf(fd, "%s (%s)\n", exception->message(), exception->klass()->class_name());
}

void handle_top_level_exception(Env *env, ExceptionValue *exception, bool run_exit_handlers) {
    if (exception->is_a(env, env->Object()->const_get(env, "SystemExit", true)->as_class())) {
        Value *status_obj = exception->ivar_get(env, "@status");
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (status_obj->type() == Value::Type::Integer) {
            int64_t val = status_obj->as_integer()->to_int64_t();
            if (val >= 0 && val <= 255) {
                exit(val);
            } else {
                exit(1);
            }
        } else {
            exit(1);
        }
    } else {
        print_exception_with_backtrace(env, exception);
    }
}

ArrayValue *to_ary(Env *env, Value *obj, bool raise_for_non_array) {
    if (obj->is_array()) {
        return obj->as_array();
    } else if (obj->respond_to(env, "to_ary")) {
        Value *ary = obj->send(env, "to_ary");
        if (ary->is_array()) {
            return ary->as_array();
        } else if (ary->is_nil() || !raise_for_non_array) {
            ary = new ArrayValue { env };
            ary->as_array()->push(obj);
            return ary->as_array();
        } else {
            const char *class_name = obj->klass()->class_name();
            NAT_RAISE(env, "TypeError", "can't convert %s to Array (%s#to_ary gives %s)", class_name, class_name, ary->klass()->class_name());
        }
    } else {
        ArrayValue *ary = new ArrayValue { env };
        ary->push(obj);
        return ary;
    }
}

static Value *splat_value(Env *env, Value *value, ssize_t index, ssize_t offset_from_end) {
    ArrayValue *splat = new ArrayValue { env };
    if (value->is_array() && index < value->as_array()->size() - offset_from_end) {
        for (ssize_t s = index; s < value->as_array()->size() - offset_from_end; s++) {
            splat->push((*value->as_array())[s]);
        }
    }
    return splat;
}

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    bool has_default = default_value != env->nil_obj();
    bool defaults_on_left = !defaults_on_right;
    int required_count = total_count - default_count;
    Value *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);

        if (splat && i == path_size - 1) {
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                assert(return_value->as_array()->size() <= NAT_MAX_INT);
                int64_t ary_len = return_value->as_array()->size();

                int first_required = default_count;
                int remain = ary_len - required_count;

                if (has_default && index >= remain && index < first_required && defaults_on_left) {
                    // this is an arg with a default value;
                    // not enough values to fill all the required args and this one
                    return default_value;
                }

                if (i == 0 && path_size == 1) {
                    // shift index left if needed
                    int extra_count = ary_len - required_count;
                    if (defaults_on_left && extra_count > 0 && default_count >= extra_count && index >= extra_count) {
                        index -= (default_count - extra_count);
                    } else if (ary_len <= required_count && defaults_on_left) {
                        index -= (default_count);
                    }
                }

                if (index < 0) {
                    // negative offset index should go from the right
                    if (ary_len >= total_count) {
                        index = ary_len + index;
                    } else {
                        // not enough values to fill from the right
                        // also, assume there is a splat prior to this index
                        index = total_count - 1 + index;
                    }
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = (*return_value->as_array())[index];

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    Value *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);
        if (splat && i == path_size - 1) {
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                assert(return_value->as_array()->size() <= NAT_MAX_INT);
                int64_t ary_len = return_value->as_array()->size();

                if (index < 0) {
                    // negative offset index should go from the right
                    index = ary_len + index;
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = (*return_value->as_array())[index];

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value) {
    return kwarg_value_by_name(env, args->as_array(), name, default_value);
}

Value *kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, Value *default_value) {
    Value *hash;
    if (args->size() == 0) {
        hash = new HashValue { env };
    } else {
        hash = (*args)[args->size() - 1];
        if (hash->type() != Value::Type::Hash) {
            hash = new HashValue { env };
        }
    }
    Value *value = hash->as_hash()->get(env, SymbolValue::intern(env, name));
    if (!value) {
        if (default_value) {
            return default_value;
        } else {
            NAT_RAISE(env, "ArgumentError", "missing keyword: :%s", name);
        }
    }
    return value;
}

ArrayValue *args_to_array(Env *env, ssize_t argc, Value **args) {
    ArrayValue *ary = new ArrayValue { env };
    for (ssize_t i = 0; i < argc; i++) {
        ary->push(args[i]);
    }
    return ary;
}

// much like args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call to_ary on the first arg and return that
ArrayValue *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args) {
    if (argc == 1 && signature_size > 1) {
        return to_ary(env, args[0], true);
    }
    return args_to_array(env, argc, args);
}

void arg_spread(Env *env, ssize_t argc, Value **args, char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    ssize_t len = strlen(arrangement);
    ssize_t arg_index = 0;
    Value *obj;
    bool *bool_ptr;
    int *int_ptr;
    const char **str_ptr;
    void **void_ptr;
    Value **obj_ptr;
    for (ssize_t i = 0; i < len; i++) {
        char c = arrangement[i];
        switch (c) {
        case 'o':
            obj_ptr = va_arg(va_args, Value **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            *obj_ptr = obj;
            break;
        case 'i':
            int_ptr = va_arg(va_args, int *);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            NAT_ASSERT_TYPE(obj, Value::Type::Integer, "Integer");
            *int_ptr = obj->as_integer()->to_int64_t();
            break;
        case 's':
            str_ptr = va_arg(va_args, const char **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            if (obj == env->nil_obj()) {
                *str_ptr = nullptr;
            } else {
                NAT_ASSERT_TYPE(obj, Value::Type::String, "String");
            }
            *str_ptr = obj->as_string()->c_str();
            break;
        case 'b':
            bool_ptr = va_arg(va_args, bool *);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            *bool_ptr = obj->is_truthy();
            break;
        case 'v':
            void_ptr = va_arg(va_args, void **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            obj = obj->ivar_get(env, "@_ptr");
            assert(obj->type() == Value::Type::VoidP);
            *void_ptr = obj->as_void_p()->void_ptr();
            break;
        default:
            fprintf(stderr, "Unknown arg spread arrangement specifier: %%%c", c);
            abort();
        }
    }
    va_end(va_args);
}

std::pair<Value *, Value *> coerce(Env *env, Value *lhs, Value *rhs) {
    if (lhs->respond_to(env, "coerce")) {
        Value *coerced = lhs->send(env, "coerce", 1, &rhs, nullptr);
        if (!coerced->is_array()) {
            NAT_RAISE(env, "TypeError", "coerce must return [x, y]");
        }
        lhs = (*coerced->as_array())[0];
        rhs = (*coerced->as_array())[1];
        return { lhs, rhs };
    } else {
        return { rhs, lhs };
    }
}

void copy_hashmap(struct hashmap &dest, const struct hashmap &source) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&source); iter; iter = hashmap_iter_next(&source, iter)) {
        char *name = (char *)hashmap_iter_get_key(iter);
        Value *value = (Value *)hashmap_iter_get_data(iter);
        hashmap_put(&dest, name, value);
    }
}

char *zero_string(int size) {
    char *buf = new char[size + 1];
    memset(buf, '0', size);
    buf[size] = 0;
    return buf;
}

Block *proc_to_block_arg(Env *env, Value *proc_or_nil) {
    if (proc_or_nil->is_nil()) {
        return nullptr;
    }
    return proc_or_nil->to_proc(env)->block();
}

}
