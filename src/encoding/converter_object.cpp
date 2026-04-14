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

Value EncodingConverterObject::initialize(Env *env, Args &&args) {
    auto kwargs = args.pop_keyword_hash();

    args.ensure_argc_between(env, 2, 3);

    m_source_encoding = EncodingObject::find(env, args.at(0)).as_encoding();
    m_destination_encoding = EncodingObject::find(env, args.at(1)).as_encoding();

    if (args.size() > 2) {
        auto val = args.at(2);
        if (val.is_integer())
            m_flags = val.integer_or_raise(env).to_nat_int_t();
        else
            apply_options_hash(env, val.to_hash(env));
    }

    if (kwargs)
        apply_options_hash(env, kwargs);

    if (!m_replacement_str)
        set_default_replacement();
    return this;
}

void EncodingConverterObject::apply_options_hash(Env *env, HashObject *hash) {
    auto invalid = hash->remove(env, "invalid"_s);
    if (invalid && invalid.value() == "replace"_s)
        m_flags |= ECONV_INVALID_REPLACE;

    auto undef = hash->remove(env, "undef"_s);
    if (undef && undef.value() == "replace"_s)
        m_flags |= ECONV_UNDEF_REPLACE;

    auto replace = hash->remove(env, "replace"_s);
    if (replace && !replace->is_nil())
        m_replacement_str = replace->to_str(env);

    auto newline = hash->remove(env, "newline"_s);
    if (newline) {
        if (newline.value() == "universal"_s)
            m_flags |= ECONV_UNIVERSAL_NEWLINE_DECORATOR;
        else if (newline.value() == "crlf"_s)
            m_flags |= ECONV_CRLF_NEWLINE_DECORATOR;
        else if (newline.value() == "cr"_s)
            m_flags |= ECONV_CR_NEWLINE_DECORATOR;
        else if (newline.value() == "lf"_s)
            m_flags |= ECONV_LF_NEWLINE_DECORATOR;
    }

    auto xml = hash->remove(env, "xml"_s);
    if (xml) {
        if (xml.value() == "text"_s)
            m_flags |= ECONV_XML_TEXT_DECORATOR;
        else if (xml.value() == "attr"_s)
            m_flags |= ECONV_XML_ATTR_CONTENT_DECORATOR;
    }
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
