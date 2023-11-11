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

Value YAML_dump(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto value = args.at(0);

    yaml_emitter_t emitter;
    yaml_event_t event;
    unsigned char buf[16384];
    size_t written = 0;

    yaml_emitter_initialize(&emitter);
    Defer emit_deleter { [&emitter]() { yaml_emitter_delete(&emitter); } };
    yaml_emitter_set_output_string(&emitter, buf, sizeof(buf), &written);

    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    emit(env, emitter, event);

    yaml_document_start_event_initialize(&event, nullptr, nullptr, nullptr, 0);
    emit(env, emitter, event);

    if (value->is_string()) {
        yaml_scalar_event_initialize(&event, nullptr, (yaml_char_t *)YAML_STR_TAG,
            (yaml_char_t *)(value->as_string()->c_str()), value->as_string()->bytesize(),
            1, 0, YAML_PLAIN_SCALAR_STYLE);
        emit(env, emitter, event);
    } else {
        env->raise("NotImplementedError", "TODO: Implement YAML output for {}", value->klass()->inspect_str());
    }

    yaml_document_end_event_initialize(&event, 0);
    emit(env, emitter, event);

    yaml_stream_end_event_initialize(&event);
    emit(env, emitter, event);

    return new StringObject { reinterpret_cast<char *>(buf), written };
}
