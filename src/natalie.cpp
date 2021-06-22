#include "natalie.hpp"
#include "natalie/forward.hpp"
#include "natalie/value_ptr.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <sys/wait.h>

namespace Natalie {

ValuePtr splat(Env *env, ValuePtr obj) {
    if (obj->is_array()) {
        return new ArrayValue { *obj->as_array() };
    } else {
        return to_ary(env, obj, false);
    }
}

void run_at_exit_handlers(Env *env) {
    ArrayValue *at_exit_handlers = env->global_get(SymbolValue::intern("$NAT_at_exit_handlers"))->as_array();
    assert(at_exit_handlers);
    for (int i = at_exit_handlers->size() - 1; i >= 0; i--) {
        ValuePtr proc = (*at_exit_handlers)[i];
        assert(proc);
        assert(proc->is_proc());
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->as_proc()->block(), 0, nullptr, nullptr);
    }
}

void print_exception_with_backtrace(Env *env, ExceptionValue *exception) {
    IoValue *_stderr = env->global_get(SymbolValue::intern("$stderr"))->as_io();
    int fd = _stderr->fileno();
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
    dprintf(fd, "%s (%s)\n", exception->message()->c_str(), exception->klass()->class_name_or_blank()->c_str());
}

void handle_top_level_exception(Env *env, ExceptionValue *exception, bool run_exit_handlers) {
    if (exception->is_a(env, GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern("SystemExit"))->as_class())) {
        ValuePtr status_obj = exception->ivar_get(env, SymbolValue::intern("@status"));
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (status_obj->type() == Value::Type::Integer) {
            nat_int_t val = status_obj->as_integer()->to_nat_int_t();
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

ArrayValue *to_ary(Env *env, ValuePtr obj, bool raise_for_non_array) {
    auto to_ary_symbol = SymbolValue::intern("to_ary");
    if (obj->is_array()) {
        return obj->as_array();
    } else if (obj->respond_to(env, to_ary_symbol)) {
        ValuePtr ary = obj.send(env, to_ary_symbol);
        if (ary->is_array()) {
            return ary->as_array();
        } else if (ary->is_nil() || !raise_for_non_array) {
            ary = new ArrayValue {};
            ary->as_array()->push(obj);
            return ary->as_array();
        } else {
            auto *class_name = obj->klass()->class_name_or_blank();
            env->raise("TypeError", "can't convert {} to Array ({}#to_ary gives {})", class_name, class_name, ary->klass()->class_name_or_blank());
        }
    } else {
        ArrayValue *ary = new ArrayValue {};
        ary->push(obj);
        return ary;
    }
}

static ValuePtr splat_value(Env *env, ValuePtr value, size_t index, size_t offset_from_end) {
    ArrayValue *splat = new ArrayValue {};
    if (value->is_array() && index < value->as_array()->size() - offset_from_end) {
        for (size_t s = index; s < value->as_array()->size() - offset_from_end; s++) {
            splat->push((*value->as_array())[s]);
        }
    }
    return splat;
}

ValuePtr arg_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, size_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    bool has_default = !!default_value;
    if (!default_value) default_value = NilValue::the();
    bool defaults_on_left = !defaults_on_right;
    int required_count = total_count - default_count;
    ValuePtr return_value = value;
    for (size_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);

        if (splat && i == path_size - 1) {
            va_end(args);
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                assert(return_value->as_array()->size() <= NAT_INT_MAX);
                nat_int_t ary_len = return_value->as_array()->size();

                int first_required = default_count;
                int remain = ary_len - required_count;

                if (has_default && index >= remain && index < first_required && defaults_on_left) {
                    // this is an arg with a default value;
                    // not enough values to fill all the required args and this one
                    va_end(args);
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

ValuePtr array_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, int offset_from_end, size_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    ValuePtr return_value = value;
    for (size_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);
        if (splat && i == path_size - 1) {
            va_end(args);
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                assert(return_value->as_array()->size() <= NAT_INT_MAX);
                nat_int_t ary_len = return_value->as_array()->size();

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

ValuePtr kwarg_value_by_name(Env *env, ValuePtr args, const char *name, ValuePtr default_value) {
    return kwarg_value_by_name(env, args->as_array(), name, default_value);
}

ValuePtr kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, ValuePtr default_value) {
    ValuePtr hash;
    if (args->size() == 0) {
        hash = new HashValue {};
    } else {
        hash = (*args)[args->size() - 1];
        if (hash->type() != Value::Type::Hash) {
            hash = new HashValue {};
        }
    }
    ValuePtr value = hash->as_hash()->get(env, SymbolValue::intern(name));
    if (!value) {
        if (default_value) {
            return default_value;
        } else {
            env->raise("ArgumentError", "missing keyword: :{}", name);
        }
    }
    return value;
}

ArrayValue *args_to_array(Env *env, size_t argc, ValuePtr *args) {
    ArrayValue *ary = new ArrayValue {};
    for (size_t i = 0; i < argc; i++) {
        ary->push(args[i]);
    }
    return ary;
}

// much like args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call to_ary on the first arg and return that
ArrayValue *block_args_to_array(Env *env, size_t signature_size, size_t argc, ValuePtr *args) {
    if (argc == 1 && signature_size > 1) {
        return to_ary(env, args[0], true);
    }
    return args_to_array(env, argc, args);
}

void arg_spread(Env *env, size_t argc, ValuePtr *args, const char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    size_t len = strlen(arrangement);
    size_t arg_index = 0;
    bool optional = false;
    for (size_t i = 0; i < len; i++) {
        if (arg_index >= argc && optional)
            break;
        char c = arrangement[i];
        switch (c) {
        case '|':
            optional = true;
            break;
        case 'o': {
            Value **obj_ptr = va_arg(va_args, Value **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            Value *obj = args[arg_index++].value();
            *obj_ptr = obj;
            break;
        }
        case 'i': {
            int *int_ptr = va_arg(va_args, int *);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            obj->assert_type(env, Value::Type::Integer, "Integer");
            *int_ptr = obj->as_integer()->to_nat_int_t();
            break;
        }
        case 's': {
            const char **str_ptr = va_arg(va_args, const char **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            if (obj == NilValue::the()) {
                *str_ptr = nullptr;
            } else {
                obj->assert_type(env, Value::Type::String, "String");
            }
            *str_ptr = obj->as_string()->c_str();
            break;
        }
        case 'b': {
            bool *bool_ptr = va_arg(va_args, bool *);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            *bool_ptr = obj->is_truthy();
            break;
        }
        case 'v': {
            void **void_ptr = va_arg(va_args, void **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            obj = obj->ivar_get(env, SymbolValue::intern("@_ptr"));
            assert(obj->type() == Value::Type::VoidP);
            *void_ptr = obj->as_void_p()->void_ptr();
            break;
        }
        default:
            fprintf(stderr, "Unknown arg spread arrangement specifier: %%%c", c);
            abort();
        }
    }
    va_end(va_args);
}

std::pair<ValuePtr, ValuePtr> coerce(Env *env, ValuePtr lhs, ValuePtr rhs) {
    auto coerce_symbol = SymbolValue::intern("coerce");
    if (lhs->respond_to(env, coerce_symbol)) {
        ValuePtr coerced = lhs.send(env, coerce_symbol, 1, &rhs, nullptr);
        if (!coerced->is_array()) {
            env->raise("TypeError", "coerce must return [x, y]");
        }
        lhs = (*coerced->as_array())[0];
        rhs = (*coerced->as_array())[1];
        return { lhs, rhs };
    } else {
        return { rhs, lhs };
    }
}

char *zero_string(int size) {
    char *buf = new char[size + 1];
    memset(buf, '0', size);
    buf[size] = 0;
    return buf;
}

Block *proc_to_block_arg(Env *env, ValuePtr proc_or_nil) {
    if (proc_or_nil->is_nil()) {
        return nullptr;
    }
    return proc_or_nil->to_proc(env)->block();
}

#define NAT_SHELL_READ_BYTES 1024

ValuePtr shell_backticks(Env *env, ValuePtr command) {
    command->assert_type(env, Value::Type::String, "String");
    int pid;
    auto process = popen2(command->as_string()->c_str(), "r", pid);
    if (!process)
        env->raise_errno();
    char buf[NAT_SHELL_READ_BYTES];
    auto result = fgets(buf, NAT_SHELL_READ_BYTES, process);
    StringValue *out;
    if (result) {
        out = new StringValue { buf };
        while (1) {
            result = fgets(buf, NAT_SHELL_READ_BYTES, process);
            if (!result) break;
            out->append(env, buf);
        }
    } else {
        out = new StringValue {};
    }
    int status = pclose2(process, pid);
    set_status_object(env, pid, status);
    return out;
}

// https://stackoverflow.com/a/26852210/197498
FILE *popen2(const char *command, const char *type, int &pid) {
    pid_t child_pid;
    int fd[2];
    if (pipe(fd) != 0) {
        fprintf(stderr, "Unable to open pipe (errno=%d)", errno);
        abort();
    }

    const int read = 0;
    const int write = 1;

    bool is_read = strcmp(type, "r") == 0;

    if ((child_pid = fork()) == -1) {
        perror("fork");
        exit(1);
    }

    // child process
    if (child_pid == 0) {
        if (is_read) {
            close(fd[read]); // close the READ end of the pipe since the child's fd is write-only
            dup2(fd[write], 1); // redirect stdout to pipe
        } else {
            close(fd[write]); // close the WRITE end of the pipe since the child's fd is read-only
            dup2(fd[read], 0); // redirect stdin to pipe
        }

        setpgid(child_pid, child_pid); // needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        exit(0);
    } else {
        if (is_read) {
            close(fd[write]); // close the WRITE end of the pipe since parent's fd is read-only
        } else {
            close(fd[read]); // close the READ end of the pipe since parent's fd is write-only
        }
    }

    pid = child_pid;

    if (is_read)
        return fdopen(fd[read], "r");

    return fdopen(fd[write], "w");
}

// https://stackoverflow.com/a/26852210/197498
int pclose2(FILE *fp, pid_t pid) {
    int stat;

    fclose(fp);
    while (waitpid(pid, &stat, 0) == -1) {
        if (errno != EINTR) {
            stat = -1;
            break;
        }
    }

    return stat;
}

void set_status_object(Env *env, int pid, int status) {
    auto status_obj = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Process"))->const_fetch(SymbolValue::intern("Status")).send(env, "new");
    status_obj->ivar_set(env, SymbolValue::intern("@to_i"), ValuePtr::integer(status));
    status_obj->ivar_set(env, SymbolValue::intern("@exitstatus"), ValuePtr::integer(WEXITSTATUS(status)));
    status_obj->ivar_set(env, SymbolValue::intern("@pid"), ValuePtr::integer(pid));
    env->global_set(SymbolValue::intern("$?"), status_obj);
}

const char *int_to_hex_string(nat_int_t num, bool capitalize) {
    if (num == 0) {
        return strdup("0");
    } else {
        char buf[100]; // ought to be enough for anybody ;-)
        if (capitalize) {
            snprintf(buf, 100, "0X%" PRIX64, num);
        } else {
            snprintf(buf, 100, "0x%" PRIx64, num);
        }
        return strdup(buf);
    }
}

ValuePtr super(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    auto current_method = env->current_method();
    auto super_method = self->klass()->find_method(env, SymbolValue::intern(env->current_method()->name()), nullptr, current_method);
    if (!super_method)
        env->raise("NoMethodError", "super: no superclass method `{}' for {}", current_method->name(), self->inspect_str(env));
    assert(super_method != current_method);
    return super_method->call(env, self, argc, args, block);
}

}
