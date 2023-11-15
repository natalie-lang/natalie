#include "natalie.hpp"

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

int main(int argc, char *argv[]) {
#ifdef NAT_NATIVE_PROFILER
    NativeProfiler::enable();
#endif

    Heap::the().set_start_of_stack(__builtin_frame_address(0));

    setvbuf(stdout, nullptr, _IOLBF, 1024);

    Env *env = ::build_top_env();
    FiberObject::build_main_fiber(Heap::the().start_of_stack());
    ThreadObject::build_main_thread(Heap::the().start_of_stack());

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
