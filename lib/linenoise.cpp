#include "../lib/linenoise/linenoise.h"
#include "../lib/linenoise/linenoise.c"
#include "natalie.hpp"

using namespace Natalie;

Value Linenoise_add_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto line = args[0]->as_string_or_raise(env)->string();

    linenoiseHistoryAdd(line.c_str());

    return args[0];
}

Value Linenoise_clear_screen(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    linenoiseClearScreen();
    return NilObject::the();
}

Value Linenoise_get_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    char **history = nullptr;
    int history_len = linenoiseHistoryGet(&history);

    auto ary = new ArrayObject {};
    if (history) {
        for (int i = 0; i < history_len; i++)
            ary->push(new StringObject { history[i] });
    }

    return ary;
}

Value Linenoise_get_multi_line(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    return bool_object(linenoiseGetMultiLine());
}

Value Linenoise_load_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto path = args[0]->as_string_or_raise(env)->string();

    linenoiseHistoryLoad(path.c_str());

    return NilObject::the();
}

Value Linenoise_readline(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);

    auto prompt = args[0]->as_string_or_raise(env)->string();

    auto line = linenoise(prompt.c_str());

    if (!line)
        return NilObject::the();

    return new StringObject { line };
}

Value Linenoise_save_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto path = args[0]->as_string_or_raise(env)->string();

    linenoiseHistorySave(path.c_str());

    return NilObject::the();
}

static void completion_callback(const char *edit_buffer, linenoiseCompletions *completions) {
    Env e {};
    auto proc = GlobalEnv::the()->Object()->const_fetch("Linenoise"_s)->as_module()->ivar_get(&e, "@completion_callback"_s)->as_proc();
    auto edit_buffer_string = new StringObject { edit_buffer };
    auto env = proc->env();
    auto ary = proc->send(env, "call"_s, { edit_buffer_string })->as_array_or_raise(env);
    for (auto &completion : *ary)
        linenoiseAddCompletion(completions, completion->as_string()->c_str());
}

Value Linenoise_set_completion_callback(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    args[0]->assert_type(env, Object::Type::Proc, "Proc");
    auto proc = args[0]->as_proc();

    self->ivar_set(env, "@completion_callback"_s, proc);

    linenoiseSetCompletionCallback(completion_callback);

    return NilObject::the();
}

static void free_hints_callback(void *hint) {
    free(hint);
}

static char *hints_callback(const char *buf, int *color, int *bold) {
    Env e {};
    auto proc = GlobalEnv::the()->Object()->const_fetch("Linenoise"_s)->as_module()->ivar_get(&e, "@hints_callback"_s)->as_proc();
    auto buf_string = new StringObject { buf };
    auto env = proc->env();

    auto ret = proc->send(env, "call"_s, { buf_string });
    if (ret->is_nil())
        return nullptr;

    auto ary = ret->as_array_or_raise(env);
    if (ary->size() < 2 || ary->size() > 3)
        env->raise("ArgumentError", "hints callback must return an array of 2 or 3 elements");

    auto hint = ary->at(0)->as_string_or_raise(env)->c_str();
    *color = ary->at(1)->as_integer_or_raise(env)->to_nat_int_t();
    *bold = ary->size() == 3 && ary->at(2)->is_truthy() ? 1 : 0;

    return strdup(hint);
}

Value Linenoise_set_hints_callback(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    args[0]->assert_type(env, Object::Type::Proc, "Proc");
    auto proc = args[0]->as_proc();

    self->ivar_set(env, "@hints_callback"_s, proc);

    linenoiseSetHintsCallback(hints_callback);
    linenoiseSetFreeHintsCallback(free_hints_callback);

    return NilObject::the();
}

static const char *highlight_callback(const char *edit_buffer, int *length) {
    Env e {};
    auto proc = GlobalEnv::the()->Object()->const_fetch("Linenoise"_s)->as_module()->ivar_get(&e, "@highlight_callback"_s)->as_proc();
    auto edit_buffer_string = new StringObject { edit_buffer };
    auto env = proc->env();
    auto string = proc->send(env, "call"_s, { edit_buffer_string })->as_string_or_raise(env);
    *length = string->length();
    return string->c_str();
}

Value Linenoise_set_highlight_callback(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    args[0]->assert_type(env, Object::Type::Proc, "Proc");
    auto proc = args[0]->as_proc();

    self->ivar_set(env, "@highlight_callback"_s, proc);

    linenoiseSetHighlightCallback(highlight_callback);

    return NilObject::the();
}

Value Linenoise_set_history(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto ary = args[0]->as_array_or_raise(env);

    linenoiseHistoryFree();

    for (auto item : *ary)
        linenoiseHistoryAdd(item->as_string_or_raise(env)->c_str());

    return ary;
}

Value Linenoise_set_history_max_len(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto length = args[0]->as_integer_or_raise(env)->to_nat_int_t();
    linenoiseHistorySetMaxLen(length);
    return Value::integer(length);
}

Value Linenoise_set_multi_line(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto enabled = args[0]->is_truthy();
    linenoiseSetMultiLine(enabled);
    return bool_object(enabled);
}

void init_linenoise(Env *env, Value self) {
    auto Linenoise = new ModuleObject { "Linenoise" };
    GlobalEnv::the()->Object()->const_set("Linenoise"_s, Linenoise);

    Linenoise->define_singleton_method(env, "add_history"_s, Linenoise_add_history, 1);
    Linenoise->define_singleton_method(env, "clear_screen"_s, Linenoise_clear_screen, 0);
    Linenoise->define_singleton_method(env, "completion_callback="_s, Linenoise_set_completion_callback, 1);
    Linenoise->define_singleton_method(env, "highlight_callback="_s, Linenoise_set_highlight_callback, 1);
    Linenoise->define_singleton_method(env, "hints_callback="_s, Linenoise_set_hints_callback, 1);
    Linenoise->define_singleton_method(env, "history"_s, Linenoise_get_history, 0);
    Linenoise->define_singleton_method(env, "history="_s, Linenoise_set_history, 1);
    Linenoise->define_singleton_method(env, "history_max_len="_s, Linenoise_set_history_max_len, 1);
    Linenoise->define_singleton_method(env, "load_history"_s, Linenoise_load_history, 1);
    Linenoise->define_singleton_method(env, "multi_line"_s, Linenoise_get_multi_line, 0);
    Linenoise->define_singleton_method(env, "multi_line="_s, Linenoise_set_multi_line, 1);
    Linenoise->define_singleton_method(env, "readline"_s, Linenoise_readline, 1);
    Linenoise->define_singleton_method(env, "save_history"_s, Linenoise_save_history, 1);
}
