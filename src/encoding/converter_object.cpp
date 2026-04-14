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

    if (m_source_encoding == m_destination_encoding)
        env->raise(fetch_nested_const({ "Encoding"_s, "ConverterNotFoundError"_s }).as_class(),
            "code converter not found ({} to {})", m_source_encoding->name_string(), m_destination_encoding->name_string());

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

    auto universal_newline = hash->remove(env, "universal_newline"_s);
    if (universal_newline && universal_newline->is_truthy())
        m_flags |= ECONV_UNIVERSAL_NEWLINE_DECORATOR;

    auto crlf_newline = hash->remove(env, "crlf_newline"_s);
    if (crlf_newline && crlf_newline->is_truthy())
        m_flags |= ECONV_CRLF_NEWLINE_DECORATOR;

    auto cr_newline = hash->remove(env, "cr_newline"_s);
    if (cr_newline && cr_newline->is_truthy())
        m_flags |= ECONV_CR_NEWLINE_DECORATOR;

    auto lf_newline = hash->remove(env, "lf_newline"_s);
    if (lf_newline && lf_newline->is_truthy())
        m_flags |= ECONV_LF_NEWLINE_DECORATOR;
}

Value EncodingConverterObject::inspect(Env *env) const {
    return StringObject::format(
        "#<Encoding::Converter: {} to {}>",
        m_source_encoding->name_string(),
        m_destination_encoding->name_string());
}

Value EncodingConverterObject::replacement(Env *env) const {
    return m_replacement_str;
}

Value EncodingConverterObject::set_replacement(Env *env, Value str) {
    m_replacement_str = str.to_str(env);
    return m_replacement_str;
}

Value EncodingConverterObject::convert(Env *env, Value source) {
    if (m_finished)
        env->raise("ArgumentError", "converter already finished");

    auto source_str = source.to_str(env);
    String input;
    if (!m_input_buffer.is_empty()) {
        input.append(m_input_buffer);
        m_input_buffer = String();
    }
    input.append(source_str->string());
    String output;
    size_t input_pos = 0;

    auto result = do_convert(env, input, &input_pos, &output, ECONV_PARTIAL_INPUT);

    // Buffer incomplete bytes for next call
    if (result == EconvResult::SourceBufferEmpty && !m_error_bytes.is_empty())
        m_input_buffer = m_error_bytes;

    switch (result) {
    case EconvResult::SourceBufferEmpty:
    case EconvResult::Finished:
        break;
    case EconvResult::UndefinedConversion: {
        auto message = StringObject::format(
            "U+{} from {} to {}",
            String::hex(m_error_codepoint, String::HexFormat::Uppercase),
            m_source_encoding->name_string(),
            m_destination_encoding->name_string());
        env->raise(fetch_nested_const({ "Encoding"_s, "UndefinedConversionError"_s }).as_class(), message);
        break;
    }
    case EconvResult::InvalidByteSequence: {
        env->raise(fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s }).as_class(),
            "invalid byte sequence in {}", m_source_encoding->name_string());
        break;
    }
    default:
        break;
    }

    m_started = true;
    return StringObject::create(std::move(output), m_destination_encoding);
}

Value EncodingConverterObject::finish(Env *env) {
    m_finished = true;

    if (!m_input_buffer.is_empty()) {
        String output;
        size_t input_pos = 0;
        auto result = do_convert(env, m_input_buffer, &input_pos, &output, 0);
        m_input_buffer = String();

        if (result == EconvResult::IncompleteInput) {
            env->raise(fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s }).as_class(),
                "incomplete byte sequence on {}", m_source_encoding->name_string());
        }

        if (!output.is_empty())
            return StringObject::create(std::move(output), m_destination_encoding);
    }

    return StringObject::create("", m_destination_encoding);
}

Value EncodingConverterObject::primitive_convert(Env *env, Args &&args) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 2, 4);

    Value source_val = args.at(0);
    auto dest_str = args.at(1).to_str(env);

    if (dest_str->is_frozen())
        env->raise("FrozenError", "can't modify frozen String: \"{}\"", dest_str->string());

    nat_int_t dest_offset = -1;
    if (args.size() > 2 && !args.at(2).is_nil())
        dest_offset = IntegerMethods::convert_to_nat_int_t(env, args.at(2));

    ssize_t max_output = -1;
    if (args.size() > 3 && !args.at(3).is_nil())
        max_output = IntegerMethods::convert_to_nat_int_t(env, args.at(3));

    int convert_flags = 0;
    if (kwargs) {
        auto partial = kwargs->remove(env, "partial_input"_s);
        if (partial && partial->is_truthy())
            convert_flags |= ECONV_PARTIAL_INPUT;
        auto after = kwargs->remove(env, "after_output"_s);
        if (after && after->is_truthy())
            convert_flags |= ECONV_AFTER_OUTPUT;
    }

    if (dest_offset >= 0) {
        if ((size_t)dest_offset > dest_str->bytesize())
            env->raise("ArgumentError", "destination byteoffset is too big");
        dest_str->truncate(dest_offset);
    }

    // Build effective input: prepend read-again bytes from previous call
    String input;
    StringObject *source_str = nullptr;
    size_t readagain_prepended = m_readagain_bytes.length();
    if (readagain_prepended > 0) {
        input.append(m_readagain_bytes);
        m_readagain_bytes = String();
    }

    if (!source_val.is_nil()) {
        source_str = source_val.to_str(env);
        input.append(source_str->string());
    }

    // Flush any buffered output from previous call
    String output;
    if (!m_output_buffer.is_empty()) {
        output.append(m_output_buffer);
        m_output_buffer = String();
    }

    size_t input_pos = 0;
    auto result = do_convert(env, input, &input_pos, &output, convert_flags, max_output);

    // Update source buffer: remove consumed bytes
    if (source_str) {
        size_t consumed_from_source;
        if (input_pos > readagain_prepended)
            consumed_from_source = input_pos - readagain_prepended;
        else
            consumed_from_source = 0;
        if (consumed_from_source >= source_str->length())
            source_str->truncate(0);
        else
            source_str->set_str(source_str->string().substring(consumed_from_source));
    }

    // Write output to destination buffer
    if (max_output >= 0 && (ssize_t)output.length() > max_output) {
        // Buffer overflow: write what fits, buffer the rest
        dest_str->append(output.substring(0, max_output));
        m_output_buffer = output.substring(max_output);
    } else {
        dest_str->append(output);
    }
    dest_str->set_encoding(m_destination_encoding);

    m_started = true;
    return result_to_symbol(result);
}

Value EncodingConverterObject::primitive_errinfo(Env *env) const {
    auto result_sym = result_to_symbol(m_last_result);
    auto ary = ArrayObject::create();

    ary->push(result_sym);

    switch (m_last_result) {
    case EconvResult::InvalidByteSequence:
    case EconvResult::UndefinedConversion:
    case EconvResult::IncompleteInput: {
        ary->push(StringObject::create(m_error_source_encoding_name, Encoding::US_ASCII));
        ary->push(StringObject::create(m_error_dest_encoding_name, Encoding::US_ASCII));
        ary->push(StringObject::create(m_error_bytes, Encoding::ASCII_8BIT));
        ary->push(StringObject::create(m_last_readagain_bytes, Encoding::ASCII_8BIT));
        break;
    }
    default:
        ary->push(Value::nil());
        ary->push(Value::nil());
        ary->push(Value::nil());
        ary->push(Value::nil());
        break;
    }

    return ary;
}

Value EncodingConverterObject::convpath(Env *env) const {
    auto ary = ArrayObject::create();
    bool source_is_utf8 = (m_source_encoding->num() == Encoding::UTF_8);
    bool dest_is_utf8 = (m_destination_encoding->num() == Encoding::UTF_8);

    if (source_is_utf8 || dest_is_utf8 || m_source_encoding == m_destination_encoding) {
        ary->push(ArrayObject::create({ m_source_encoding, m_destination_encoding }));
    } else {
        auto utf8 = EncodingObject::get(Encoding::UTF_8);
        ary->push(ArrayObject::create({ m_source_encoding, utf8 }));
        ary->push(ArrayObject::create({ utf8, m_destination_encoding }));
    }

    if (m_flags & ECONV_UNIVERSAL_NEWLINE_DECORATOR)
        ary->push(StringObject::create("universal_newline", Encoding::US_ASCII));
    if (m_flags & ECONV_CRLF_NEWLINE_DECORATOR)
        ary->push(StringObject::create("crlf_newline", Encoding::US_ASCII));
    if (m_flags & ECONV_CR_NEWLINE_DECORATOR)
        ary->push(StringObject::create("cr_newline", Encoding::US_ASCII));
    if (m_flags & ECONV_LF_NEWLINE_DECORATOR)
        ary->push(StringObject::create("lf_newline", Encoding::US_ASCII));
    if (m_flags & ECONV_XML_TEXT_DECORATOR)
        ary->push(StringObject::create("xml_text_escape", Encoding::US_ASCII));
    if (m_flags & ECONV_XML_ATTR_CONTENT_DECORATOR)
        ary->push(StringObject::create("xml_attr_content_escape", Encoding::US_ASCII));

    return ary;
}

Value EncodingConverterObject::putback(Env *env, Optional<Value> max_bytes) {
    size_t n = m_readagain_bytes.length();
    if (max_bytes && !max_bytes.value().is_nil()) {
        size_t limit = IntegerMethods::convert_to_nat_int_t(env, max_bytes.value());
        if (limit < n) n = limit;
    }

    if (n == 0)
        return StringObject::create("", m_source_encoding);

    auto result = StringObject::create(m_readagain_bytes.c_str(), n, m_source_encoding);
    if (n >= m_readagain_bytes.length())
        m_readagain_bytes = String();
    else
        m_readagain_bytes = m_readagain_bytes.substring(n);
    return result;
}

Value EncodingConverterObject::last_error(Env *env) const {
    switch (m_last_result) {
    case EconvResult::InvalidByteSequence: {
        auto klass = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s }).as_class();
        auto message = StringObject::format("invalid byte sequence in {}", m_error_source_encoding_name);
        return ExceptionObject::create(klass, message);
    }
    case EconvResult::IncompleteInput: {
        auto klass = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s }).as_class();
        auto message = StringObject::format("incomplete byte sequence on {}", m_error_source_encoding_name);
        return ExceptionObject::create(klass, message);
    }
    case EconvResult::UndefinedConversion: {
        auto klass = fetch_nested_const({ "Encoding"_s, "UndefinedConversionError"_s }).as_class();
        auto message = StringObject::format(
            "U+{} from {} to {}",
            String::hex(m_error_codepoint, String::HexFormat::Uppercase),
            m_error_source_encoding_name,
            m_error_dest_encoding_name);
        return ExceptionObject::create(klass, message);
    }
    default:
        return Value::nil();
    }
}

Value EncodingConverterObject::insert_output(Env *env, Value str) {
    // TODO: implement
    env->raise("NotImplementedError", "Encoding::Converter#insert_output is not yet implemented");
}

Value EncodingConverterObject::asciicompat_encoding(Env *env, Value encoding) {
    auto enc = EncodingObject::find(env, encoding);
    if (enc.is_nil())
        return Value::nil();
    auto enc_obj = enc.as_encoding();
    if (enc_obj->is_ascii_compatible())
        return Value::nil();
    // For ASCII-incompatible encodings, return UTF-8 as the compatible encoding
    return EncodingObject::get(Encoding::UTF_8);
}

Value EncodingConverterObject::search_convpath(Env *env, Args &&args) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_is(env, 2);

    auto source = EncodingObject::find(env, args.at(0)).as_encoding();
    auto dest = EncodingObject::find(env, args.at(1)).as_encoding();

    int flags = 0;
    if (kwargs) {
        auto crlf = kwargs->remove(env, "crlf_newline"_s);
        if (crlf && crlf->is_truthy())
            flags |= ECONV_CRLF_NEWLINE_DECORATOR;
        auto universal = kwargs->remove(env, "universal_newline"_s);
        if (universal && universal->is_truthy())
            flags |= ECONV_UNIVERSAL_NEWLINE_DECORATOR;
        auto cr = kwargs->remove(env, "cr_newline"_s);
        if (cr && cr->is_truthy())
            flags |= ECONV_CR_NEWLINE_DECORATOR;
        auto lf = kwargs->remove(env, "lf_newline"_s);
        if (lf && lf->is_truthy())
            flags |= ECONV_LF_NEWLINE_DECORATOR;
    }

    auto ary = ArrayObject::create();
    bool source_is_utf8 = (source->num() == Encoding::UTF_8);
    bool dest_is_utf8 = (dest->num() == Encoding::UTF_8);

    if (source_is_utf8 || dest_is_utf8 || source == dest) {
        ary->push(ArrayObject::create({ source, dest }));
    } else {
        auto utf8 = EncodingObject::get(Encoding::UTF_8);
        ary->push(ArrayObject::create({ source, utf8 }));
        ary->push(ArrayObject::create({ utf8, dest }));
    }

    if (flags & ECONV_UNIVERSAL_NEWLINE_DECORATOR)
        ary->push(StringObject::create("universal_newline", Encoding::US_ASCII));
    if (flags & ECONV_CRLF_NEWLINE_DECORATOR)
        ary->push(StringObject::create("crlf_newline", Encoding::US_ASCII));
    if (flags & ECONV_CR_NEWLINE_DECORATOR)
        ary->push(StringObject::create("cr_newline", Encoding::US_ASCII));
    if (flags & ECONV_LF_NEWLINE_DECORATOR)
        ary->push(StringObject::create("lf_newline", Encoding::US_ASCII));

    return ary;
}

void EncodingConverterObject::set_error_encoding_names_for_decode() {
    m_error_source_encoding_name = m_source_encoding->name_string();
    if (m_source_encoding->num() == Encoding::UTF_8)
        m_error_dest_encoding_name = m_destination_encoding->name_string();
    else
        m_error_dest_encoding_name = String("UTF-8");
}

void EncodingConverterObject::set_error_encoding_names_for_encode() {
    if (m_source_encoding->num() == Encoding::UTF_8)
        m_error_source_encoding_name = m_source_encoding->name_string();
    else
        m_error_source_encoding_name = String("UTF-8");
    m_error_dest_encoding_name = m_destination_encoding->name_string();
}

EconvResult EncodingConverterObject::do_convert(
    Env *env,
    const String &input, size_t *input_pos,
    String *output,
    int convert_flags,
    ssize_t max_output) {

    m_error_bytes = String();
    m_readagain_bytes = String();
    m_last_readagain_bytes = String();
    m_error_codepoint = -1;
    m_error_source_encoding_name = String();
    m_error_dest_encoding_name = String();

    while (*input_pos < input.length()) {
        auto start_pos = *input_pos;
        auto [valid, length, codepoint] = m_source_encoding->next_codepoint(input, input_pos);

        if (length == 0)
            break;

        if (!valid || codepoint < 0) {
            int expected = m_source_encoding->expected_byte_count(input, start_pos);

            // Determine if this is incomplete input or an invalid byte sequence
            if (expected > length && *input_pos >= input.length()) {
                // Truncated: ran out of input bytes
                m_error_bytes = input.substring(start_pos, length);
                set_error_encoding_names_for_decode();
                if (convert_flags & ECONV_PARTIAL_INPUT) {
                    m_last_result = EconvResult::SourceBufferEmpty;
                    return m_last_result;
                }
                if (m_flags & ECONV_INVALID_REPLACE) {
                    output->append(m_replacement_str->string());
                    continue;
                }
                m_last_result = EconvResult::IncompleteInput;
                return m_last_result;
            }

            // Invalid byte sequence
            m_error_bytes = input.substring(start_pos, length);
            set_error_encoding_names_for_decode();

            // Detect read-again bytes: if the lead byte expected more bytes
            // and a non-continuation byte at input_pos caused the error
            int unit_size = m_source_encoding->code_unit_size();
            if (expected > unit_size && length < expected && *input_pos + unit_size <= input.length()) {
                m_readagain_bytes = input.substring(*input_pos, unit_size);
                m_last_readagain_bytes = m_readagain_bytes;
                (*input_pos) += unit_size;
            }

            if (m_flags & ECONV_INVALID_REPLACE) {
                output->append(m_replacement_str->string());
                continue;
            }
            m_last_result = EconvResult::InvalidByteSequence;
            return m_last_result;
        }

        auto unicode_codepoint = m_source_encoding->to_unicode_codepoint(codepoint);
        if (unicode_codepoint < 0) {
            if (m_flags & ECONV_UNDEF_REPLACE) {
                output->append(m_replacement_str->string());
                continue;
            }
            m_error_bytes = input.substring(start_pos, length);
            m_error_codepoint = codepoint;
            set_error_encoding_names_for_decode();
            m_last_result = EconvResult::UndefinedConversion;
            return m_last_result;
        }

        auto dest_codepoint = m_destination_encoding->from_unicode_codepoint(unicode_codepoint);
        if (dest_codepoint < 0) {
            if (m_flags & ECONV_UNDEF_REPLACE) {
                output->append(m_replacement_str->string());
                continue;
            }
            m_error_bytes = input.substring(start_pos, length);
            m_error_codepoint = unicode_codepoint;
            set_error_encoding_names_for_encode();
            m_last_result = EconvResult::UndefinedConversion;
            return m_last_result;
        }

        auto encoded = m_destination_encoding->encode_codepoint(dest_codepoint);

        // Check destination buffer size limit
        if (max_output >= 0 && (ssize_t)(output->length() + encoded.length()) > max_output) {
            m_last_result = EconvResult::DestinationBufferFull;
            // Buffer the encoded output for next call
            m_output_buffer.append(encoded);
            return m_last_result;
        }

        output->append(encoded);

        if (convert_flags & ECONV_AFTER_OUTPUT) {
            m_last_result = EconvResult::AfterOutput;
            return m_last_result;
        }
    }

    if (convert_flags & ECONV_PARTIAL_INPUT) {
        m_last_result = EconvResult::SourceBufferEmpty;
    } else {
        m_last_result = EconvResult::Finished;
    }
    return m_last_result;
}

}
