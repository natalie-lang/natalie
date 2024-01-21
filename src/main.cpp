#include "natalie.hpp"

#include <signal.h>

using namespace Natalie;

#ifdef NAT_PRINT_OBJECTS
#define NAT_GC_DISABLE
#endif

/*NAT_DECLARATIONS*/

extern "C" Env *build_top_env() {
    auto env = Natalie::build_top_env();
    [[maybe_unused]] Value self = GlobalEnv::the()->main_obj();
    /*NAT_OBJ_INIT*/
    return env;
}

extern "C" Object *EVAL(Env *env) {
    /*NAT_EVAL_INIT*/

    [[maybe_unused]] Value self = GlobalEnv::the()->main_obj();
    volatile bool run_exit_handlers = true;

    // kinda hacky, but needed for top-level begin/rescue
    Args args;
    Block *block = nullptr;

    try {
        // FIXME: top-level `return` in a Ruby script should probably be changed to `exit`.
        // For now, this lambda lets us return a Value from generated code without breaking the C linkage.
        auto result = [&]() -> Value {
            /*NAT_EVAL_BODY*/
            return NilObject::the();
        }();
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return result.object();
    } catch (ExceptionObject *exception) {
        handle_top_level_exception(env, exception, run_exit_handlers);
        return nullptr;
    }
}

void sigint_handler(int, siginfo_t *, void *) {
    const char *msg = "Interrupt\n";
    auto bytes_written = write(STDOUT_FILENO, msg, strlen(msg));
    if (bytes_written == -1) abort();
    exit(128 + SIGINT);
}

void sigpipe_handler(int, siginfo_t *, void *) {
    // TODO: do something here?
}

void gc_signal_handler(int signal, siginfo_t *, void *ucontext) {
    switch (signal) {
    case SIGUSR1: {
        auto thread = ThreadObject::current();
        if (!thread || thread->is_main()) return;

        auto ctx = thread->get_context();
        if (!ctx) {
            char msg[] = "Fatal: Could not get pointer for thread context.";
            assert(::write(STDERR_FILENO, msg, sizeof(msg)) != -1);
            abort();
        }
        memcpy(ctx, ucontext, sizeof(ucontext_t));
        thread->set_context_saved(true);
#ifdef NAT_DEBUG_THREADS
        char msg[] = "THREAD DEBUG: Thread paused\n";
        assert(::write(STDERR_FILENO, msg, sizeof(msg)) != -1);
#endif
        pause();
        break;
    }
    case SIGUSR2:
#ifdef NAT_DEBUG_THREADS
        char msg2[] = "THREAD DEBUG: Thread woke up\n";
        assert(::write(STDERR_FILENO, msg2, sizeof(msg2)) != -1);
#endif
        break;
    }
}

void trap_signal(int signal, void (*handler)(int, siginfo_t *, void *)) {
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(signal, &sa, nullptr) == -1) {
        printf("Failed to trap %d\n", signal);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
#ifdef NAT_NATIVE_PROFILER
    NativeProfiler::enable();
#endif

    setvbuf(stdout, nullptr, _IOLBF, 1024);

    Env *env = ::build_top_env();
    ThreadObject::build_main_thread(env, __builtin_frame_address(0));

    trap_signal(SIGINT, sigint_handler);
    trap_signal(SIGPIPE, sigpipe_handler);
    trap_signal(SIGUSR1, gc_signal_handler);
    trap_signal(SIGUSR2, gc_signal_handler);

#ifndef NAT_GC_DISABLE
    Heap::the().gc_enable();
#endif
#ifdef NAT_GC_COLLECT_ALL_AT_EXIT
    Heap::the().set_collect_all_at_exit(true);
#endif

    if (argc > 0) {
        Value exe = new StringObject { argv[0] };
        env->global_set("$exe"_s, exe);
    }

    ArrayObject *ARGV = new ArrayObject { (size_t)argc };
    GlobalEnv::the()->Object()->const_set("ARGV"_s, ARGV);
    for (int i = 1; i < argc; i++) {
        ARGV->push(new StringObject { argv[i] });
    }

    auto result = EVAL(env);
    auto return_code = result ? 0 : 1;

#ifdef NAT_NATIVE_PROFILER
    NativeProfiler::the()->dump();
#endif
#ifdef NAT_PRINT_OBJECTS
    Heap::the().dump();
#endif
    clean_up_and_exit(return_code);
}
