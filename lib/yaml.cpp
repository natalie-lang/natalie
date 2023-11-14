#include "natalie.hpp"
#include <yaml.h>

using namespace Natalie;

Value init(Env *env, Value self) {
    return NilObject::the();
}

static void emit(Env *env, yaml_emitter_t &emitter, yaml_event_t &event) {
    if (!yaml_emitter_emit(&emitter, &event))
        env->raise("RuntimeError", "Error in yaml_emitter_emit");
}

static void emit_value(Env *, Value, yaml_emitter_t &, yaml_event_t &);

static void emit_value(Env *env, ArrayObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    yaml_sequence_start_event_initialize(&event, nullptr, (yaml_char_t *)YAML_SEQ_TAG,
        1, YAML_ANY_SEQUENCE_STYLE);
    emit(env, emitter, event);

    for (auto elem : *value)
        emit_value(env, elem, emitter, event);

    yaml_sequence_end_event_initialize(&event);
    emit(env, emitter, event);
}

static void emit_value(Env *env, FalseObject *, yaml_emitter_t &emitter, yaml_event_t &event) {
    const TM::String str { "false" };
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_BOOL_TAG,
        (yaml_char_t *)(str.c_str()), str.size(), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, FloatObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    const auto str = value->to_s()->as_string()->string();
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_FLOAT_TAG,
        (yaml_char_t *)(str.c_str()), str.size(), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, HashObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    yaml_mapping_start_event_initialize(&event, nullptr, (yaml_char_t *)YAML_MAP_TAG,
        1, YAML_ANY_MAPPING_STYLE);
    emit(env, emitter, event);

    for (auto elem : *value) {
        emit_value(env, elem.key, emitter, event);
        emit_value(env, elem.val, emitter, event);
    }

    yaml_mapping_end_event_initialize(&event);
    emit(env, emitter, event);
}

static void emit_value(Env *env, IntegerObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    const auto str = value->to_s();
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_INT_TAG,
        (yaml_char_t *)(str.c_str()), str.size(), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, NilObject *, yaml_emitter_t &emitter, yaml_event_t &event) {
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_NULL_TAG,
        (yaml_char_t *)"", 0, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, RegexpObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    auto str = value->inspect_str(env);
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)"!ruby/regexp",
        (yaml_char_t *)(str.c_str()), str.size(), 0, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, StringObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_STR_TAG,
        (yaml_char_t *)(value->as_string()->c_str()), value->as_string()->bytesize(),
        1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, SymbolObject *value, yaml_emitter_t &emitter, yaml_event_t &event) {
    TM::String str = value->string();
    str.prepend_char(':');
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_STR_TAG,
        (yaml_char_t *)(str.c_str()), str.size(), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, TrueObject *, yaml_emitter_t &emitter, yaml_event_t &event) {
    const TM::String str { "true" };
    yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_BOOL_TAG,
        (yaml_char_t *)(str.c_str()), str.size(), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    emit(env, emitter, event);
}

static void emit_value(Env *env, Value value, yaml_emitter_t &emitter, yaml_event_t &event) {
    if (value->is_array()) {
        emit_value(env, value->as_array(), emitter, event);
    } else if (value->is_false()) {
        emit_value(env, value->as_false(), emitter, event);
    } else if (value->is_float()) {
        emit_value(env, value->as_float(), emitter, event);
    } else if (value->is_hash()) {
        emit_value(env, value->as_hash(), emitter, event);
    } else if (value->is_integer()) {
        emit_value(env, value->as_integer(), emitter, event);
    } else if (value->is_nil()) {
        emit_value(env, value->as_nil(), emitter, event);
    } else if (value->is_regexp()) {
        emit_value(env, value->as_regexp(), emitter, event);
    } else if (value->is_string()) {
        emit_value(env, value->as_string(), emitter, event);
    } else if (value->is_symbol()) {
        emit_value(env, value->as_symbol(), emitter, event);
    } else if (value->is_true()) {
        emit_value(env, value->as_true(), emitter, event);
    } else if (GlobalEnv::the()->Object()->defined(env, "Date"_s, false) && value->is_a(env, GlobalEnv::the()->Object()->const_get("Date"_s)->as_class())) {
        emit_value(env, value->send(env, "to_s"_s)->as_string(), emitter, event);
    } else {
        env->raise("NotImplementedError", "TODO: Implement YAML output for {}", value->klass()->inspect_str());
    }
}

Value YAML_dump(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto value = args.at(0);

    yaml_emitter_t emitter;
    yaml_event_t event;
    unsigned char buf[16384];
    size_t written = 0;
    FILE *file = nullptr;

    yaml_emitter_initialize(&emitter);
    Defer emit_deleter { [&emitter]() { yaml_emitter_delete(&emitter); } };
    if (args.size() > 1) {
        auto io = args.at(1)->as_io();
        file = fdopen(io->fileno(env), "wb");
        yaml_emitter_set_output_file(&emitter, file);
    } else {
        yaml_emitter_set_output_string(&emitter, buf, sizeof(buf), &written);
    }

    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    emit(env, emitter, event);

    yaml_document_start_event_initialize(&event, nullptr, nullptr, nullptr, 0);
    emit(env, emitter, event);

    emit_value(env, value, emitter, event);

    yaml_document_end_event_initialize(&event, 0);
    emit(env, emitter, event);

    yaml_stream_end_event_initialize(&event);
    emit(env, emitter, event);

    if (file) {
        fflush(file);
        return args.at(1);
    }

    return new StringObject { reinterpret_cast<char *>(buf), written };
}
