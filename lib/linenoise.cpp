#include "../lib/linenoise.hpp"
#include "natalie.hpp"

using namespace Natalie;

Value Linenoise_add_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto line = args[0]->as_string_or_raise(env)->string();

    linenoise::AddHistory(line.c_str());

    return args[0];
}

Value Linenoise_load_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto path = args[0]->as_string_or_raise(env)->string();

    linenoise::LoadHistory(path.c_str());

    return NilObject::the();
}

Value Linenoise_readline(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);

    auto prompt = args[0]->as_string_or_raise(env)->string();

    std::string line;
    auto quit = linenoise::Readline(prompt.c_str(), line);

    if (quit)
        return NilObject::the();

    return new StringObject { line.c_str(), line.size() };
}

Value Linenoise_save_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto path = args[0]->as_string_or_raise(env)->string();

    linenoise::SaveHistory(path.c_str());

    return NilObject::the();
}

Value Linenoise_set_history_max_len(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto length = args[0]->as_integer_or_raise(env)->to_nat_int_t();
    linenoise::SetHistoryMaxLen(length);
    return Value::integer(length);
}

Value Linenoise_set_multi_line(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto enabled = args[0]->is_truthy();
    return bool_object(enabled);
}

Value init(Env *env, Value self) {
    auto Linenoise = new ModuleObject { "Linenoise" };
    GlobalEnv::the()->Object()->const_set("Linenoise"_s, Linenoise);

    Linenoise->define_singleton_method(env, "add_history"_s, Linenoise_add_history, 1);
    Linenoise->define_singleton_method(env, "history_max_len="_s, Linenoise_set_history_max_len, 1);
    Linenoise->define_singleton_method(env, "load_history"_s, Linenoise_load_history, 1);
    Linenoise->define_singleton_method(env, "multi_line="_s, Linenoise_set_multi_line, 1);
    Linenoise->define_singleton_method(env, "readline"_s, Linenoise_readline, 1);
    Linenoise->define_singleton_method(env, "save_history"_s, Linenoise_save_history, 1);

    return NilObject::the();
}
