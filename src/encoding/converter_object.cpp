#include "natalie.hpp"

namespace Natalie {

Value EncodingConverterObject::result_to_symbol(EconvResult result) {
    switch (result) {
    case EconvResult::InvalidByteSequence:
        return "invalid_byte_sequence"_s;
    case EconvResult::UndefinedConversion:
        return "undefined_conversion"_s;
    case EconvResult::DestinationBufferFull:
        return "destination_buffer_full"_s;
    case EconvResult::SourceBufferEmpty:
        return "source_buffer_empty"_s;
    case EconvResult::Finished:
        return "finished"_s;
    case EconvResult::AfterOutput:
        return "after_output"_s;
    case EconvResult::IncompleteInput:
        return "incomplete_input"_s;
    }
    NAT_UNREACHABLE();
}

void EncodingConverterObject::set_default_replacement() {
    if (m_destination_encoding->num() == Encoding::UTF_8)
        m_replacement_str = StringObject::create("\xEF\xBF\xBD", Encoding::UTF_8); // U+FFFD
    else
        m_replacement_str = StringObject::create("?", Encoding::US_ASCII);
}

Value EncodingConverterObject::initialize(Env *env, Value source, Value destination, Optional<Value> flags_or_options) {
    auto find_encoding = [&](Value val) -> EncodingObject * {
        if (val.is_encoding())
            return val.as_encoding();
        auto name = val.to_str(env)->string();
        auto enc = EncodingObject::find_encoding_by_name(env, name);
        if (!enc)
            env->raise("ArgumentError", "unknown encoding name - {}", name);
        return enc;
    };

    m_source_encoding = find_encoding(source);
    m_destination_encoding = find_encoding(destination);

    if (flags_or_options) {
        auto val = flags_or_options.value();
        if (val.is_integer()) {
            m_flags = val.integer_or_raise(env).to_nat_int_t();
        } else if (val.is_hash()) {
            // TODO: parse options hash
        }
    }

    set_default_replacement();
    return this;
}

Value EncodingConverterObject::inspect(Env *env) const {
    return StringObject::format(
        "#<Encoding::Converter: {} to {}>",
        m_source_encoding->name()->string(),
        m_destination_encoding->name()->string());
}

Value EncodingConverterObject::replacement(Env *env) const {
    return m_replacement_str;
}

Value EncodingConverterObject::set_replacement(Env *env, Value str) {
    m_replacement_str = str.to_str(env);
    return m_replacement_str;
}

Value EncodingConverterObject::convert(Env *env, Value source) {
    // TODO: implement using do_convert
    env->raise("NotImplementedError", "Encoding::Converter#convert is not yet implemented");
}

Value EncodingConverterObject::finish(Env *env) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#finish is not yet implemented");
}

Value EncodingConverterObject::primitive_convert(Env *env, Args &&args) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#primitive_convert is not yet implemented");
}

Value EncodingConverterObject::primitive_errinfo(Env *env) const {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#primitive_errinfo is not yet implemented");
}

Value EncodingConverterObject::convpath(Env *env) const {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#convpath is not yet implemented");
}

Value EncodingConverterObject::putback(Env *env, Optional<Value> max_bytes) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#putback is not yet implemented");
}

Value EncodingConverterObject::last_error(Env *env) const {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#last_error is not yet implemented");
}

Value EncodingConverterObject::insert_output(Env *env, Value str) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#insert_output is not yet implemented");
}

Value EncodingConverterObject::asciicompat_encoding(Env *env, Value encoding) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter.asciicompat_encoding is not yet implemented");
}

Value EncodingConverterObject::search_convpath(Env *env, Args &&args) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter.search_convpath is not yet implemented");
}

EconvResult EncodingConverterObject::do_convert(
    const String &input, size_t *input_pos,
    String *output, size_t max_output,
    int convert_flags) {
    // TODO: implement core conversion engine
    return EconvResult::Finished;
}

}
