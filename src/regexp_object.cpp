#include "natalie.hpp"

namespace Natalie {

namespace {
    struct named_captures_data {
        Env *env;
        HashObject *named_captures;
    };
};

static StringObject *regexp_stringify(const TM::String &str, const size_t start, const size_t len, StringObject *out) {
    for (size_t i = start; i < len; i++) {
        char c = str[i];
        switch (c) {
        case '/':
            out->append("\\/");
            break;
        case '\\':
            if (str[i + 1] == '/') {
                break;
            }
            out->append("\\");
            if (str[i + 1] == '\\') {
                out->append("\\");
                i++;
            }
            break;
        default:
            out->append_char(c);
        }
    }

    return out;
}

static const auto ruby_encoding_lookup = []() {
    // No constructor with std::initializer_list, use a lambda to make a const value
    auto map = Hashmap<OnigEncoding, Value>();

    map.put(ONIG_ENCODING_ASCII, EncodingObject::get(Encoding::US_ASCII));
    map.put(ONIG_ENCODING_ISO_8859_1, EncodingObject::get(Encoding::ISO_8859_1));
    map.put(ONIG_ENCODING_ISO_8859_2, EncodingObject::get(Encoding::ISO_8859_2));
    map.put(ONIG_ENCODING_ISO_8859_3, EncodingObject::get(Encoding::ISO_8859_3));
    map.put(ONIG_ENCODING_ISO_8859_4, EncodingObject::get(Encoding::ISO_8859_4));
    map.put(ONIG_ENCODING_ISO_8859_5, EncodingObject::get(Encoding::ISO_8859_5));
    map.put(ONIG_ENCODING_ISO_8859_6, EncodingObject::get(Encoding::ISO_8859_6));
    map.put(ONIG_ENCODING_ISO_8859_7, EncodingObject::get(Encoding::ISO_8859_7));
    map.put(ONIG_ENCODING_ISO_8859_8, EncodingObject::get(Encoding::ISO_8859_8));
    map.put(ONIG_ENCODING_ISO_8859_9, EncodingObject::get(Encoding::ISO_8859_9));
    map.put(ONIG_ENCODING_ISO_8859_10, EncodingObject::get(Encoding::ISO_8859_10));
    map.put(ONIG_ENCODING_ISO_8859_11, EncodingObject::get(Encoding::ISO_8859_11));
    map.put(ONIG_ENCODING_ISO_8859_13, EncodingObject::get(Encoding::ISO_8859_13));
    map.put(ONIG_ENCODING_ISO_8859_14, EncodingObject::get(Encoding::ISO_8859_14));
    map.put(ONIG_ENCODING_ISO_8859_15, EncodingObject::get(Encoding::ISO_8859_15));
    map.put(ONIG_ENCODING_ISO_8859_16, EncodingObject::get(Encoding::ISO_8859_16));
    map.put(ONIG_ENCODING_UTF_8, EncodingObject::get(Encoding::UTF_8));
    map.put(ONIG_ENCODING_UTF_16BE, EncodingObject::get(Encoding::UTF_16BE));
    map.put(ONIG_ENCODING_UTF_16LE, EncodingObject::get(Encoding::UTF_16LE));
    map.put(ONIG_ENCODING_UTF_32BE, EncodingObject::get(Encoding::UTF_32BE));
    map.put(ONIG_ENCODING_UTF_32LE, EncodingObject::get(Encoding::UTF_32LE));
    map.put(ONIG_ENCODING_EUC_JP, EncodingObject::get(Encoding::EUC_JP));
    // ONIG_ENCODING_EUC_TW has no local encoding
    // ONIG_ENCODING_EUC_KR has no local encoding
    // ONIG_ENCODING_EUC_CN has no local encoding
    map.put(ONIG_ENCODING_SHIFT_JIS, EncodingObject::get(Encoding::SHIFT_JIS));
    // ONIG_ENCODING_WINDOWS_31J has no local encoding
    // ONIG_ENCODING_KOI8_R has no local encoding
    // ONIG_ENCODING_KOI8_U has no local encoding
    map.put(ONIG_ENCODING_WINDOWS_1250, EncodingObject::get(Encoding::Windows_1250));
    map.put(ONIG_ENCODING_WINDOWS_1251, EncodingObject::get(Encoding::Windows_1251));
    map.put(ONIG_ENCODING_WINDOWS_1252, EncodingObject::get(Encoding::Windows_1252));
    // ONIG_ENCODING_WINDOWS_1253 has no local encoding
    // ONIG_ENCODING_WINDOWS_1254 has no local encoding
    // ONIG_ENCODING_WINDOWS_1257 has no local encoding
    // ONIG_ENCODING_BIG5 has no local encoding
    // ONIG_ENCODING_GB18030 has no local encoding

    return map;
}();

static const auto onig_encoding_lookup = []() {
    auto map = Hashmap<Value, OnigEncoding>();
    for (auto [onig_encoding, ruby_encoding] : ruby_encoding_lookup) {
        map.put(ruby_encoding, onig_encoding);
    }
    return map;
}();

EncodingObject *RegexpObject::onig_encoding_to_ruby_encoding(const OnigEncoding encoding) {
    auto result = ruby_encoding_lookup.get(encoding);
    if (result) return result->as_encoding();

    // Use US_ASCII as the default
    return EncodingObject::get(Encoding::US_ASCII);
}

OnigEncoding RegexpObject::ruby_encoding_to_onig_encoding(const Value encoding) {
    auto result = onig_encoding_lookup.get(encoding);
    if (result) return result;

    // Use US_ASCII as the default
    return ONIG_ENCODING_ASCII;
}

Value RegexpObject::last_match(Env *env, Value ref) {
    auto match = env->caller()->last_match();
    if (ref && match->is_match_data())
        return match->as_match_data()->ref(env, ref);
    return match;
}

Value RegexpObject::quote(Env *env, Value string) {
    if (string->is_symbol())
        string = string->to_s(env);
    auto str = string->as_string_or_raise(env);

    String out;
    for (size_t i = 0; i < str->length(); i++) {
        const char c = str->c_str()[i];
        switch (c) {
        case '\\':
        case '*':
        case '?':
        case '{':
        case '}':
        case '.':
        case '+':
        case '^':
        case '[':
        case ']':
        case '(':
        case ')':
        case '-':
        case ' ':
            out.append_char('\\');
            out.append_char(c);
            break;
        case '\n':
            out.append("\\n");
            break;
        case '\r':
            out.append("\\r");
            break;
        case '\f':
            out.append("\\f");
            break;
        case '\t':
            out.append("\\t");
            break;
        default:
            out.append_char(c);
        }
    }

    auto encoding = EncodingObject::get(Encoding::ASCII_8BIT);
    if (str->is_ascii_only())
        encoding = EncodingObject::get(Encoding::US_ASCII);
    else if (str->valid_encoding())
        encoding = str->encoding();
    return new StringObject { std::move(out), encoding };
}

Value RegexpObject::try_convert(Env *env, Value value) {
    if (!value->is_regexp() && value->respond_to(env, "to_regexp"_s)) {
        auto result = value->send(env, "to_regexp"_s);
        if (!result->is_regexp()) {
            auto value_class_name = value->klass()->name().value_or("Object");
            env->raise("TypeError", "can't convert {} to Regexp ({}#to_regexp gives {})", value_class_name, value_class_name, result->klass()->name().value_or("Object"));
        }
        return result;
    } else if (!value->is_regexp())
        return NilObject::the();
    return value;
}

Value RegexpObject::regexp_union(Env *env, Args args) {
    auto patterns = args.size() == 1 && args[0]->is_array() ? args[0]->as_array() : args.to_array();
    if (patterns->is_empty())
        return RegexpObject::literal(env, "(?!)");
    String out;
    for (auto pattern : *patterns) {
        if (!out.is_empty())
            out.append_char('|');
        if (pattern->respond_to(env, "to_regexp"_s)) {
            pattern = pattern->send(env, "to_regexp"_s);
        } else if (pattern->is_symbol()) {
            pattern = pattern->to_s(env);
        }
        if (pattern->is_regexp()) {
            if (patterns->size() == 1)
                return pattern;
            out.append(pattern->as_regexp()->to_s(env)->as_string()->string());
        } else {
            pattern = pattern->to_str(env);
            auto quoted = RegexpObject::quote(env, pattern);
            out.append(quoted->as_string()->string());
        }
    }
    return new RegexpObject { env, out };
}

Value RegexpObject::initialize(Env *env, Value pattern, Value opts) {
    assert_not_frozen(env);
    if (is_initialized())
        env->raise("TypeError", "already initialized regexp");
    if (pattern->is_regexp()) {
        auto other = pattern->as_regexp();
        initialize(env, other->pattern(), other->options());
    } else {
        nat_int_t options = 0;
        if (opts != nullptr) {
            if (opts.is_fast_integer()) {
                options = opts.get_fast_integer();
            } else if (opts->is_integer()) {
                options = opts->as_integer()->to_nat_int_t();
            } else if (opts->is_string()) {
                for (auto c : *opts->as_string()) {
                    if (c == "i") {
                        options |= RegexOpts::IgnoreCase;
                    } else if (c == "m") {
                        options |= RegexOpts::MultiLine;
                    } else if (c == "x") {
                        options |= RegexOpts::Extended;
                    } else {
                        env->raise("ArgumentError", "unknown regexp option: {}", opts->as_string()->string());
                    }
                }
            } else {
                if (!opts->is_true() && !opts->is_false() && env->global_get("$VERBOSE"_s)->is_truthy())
                    env->warn("expected true or false as ignorecase: {}", opts->inspect_str(env));
                if (opts->is_truthy())
                    options = RegexOpts::IgnoreCase;
            }
        }

        initialize(env, pattern->to_str(env)->c_str(), static_cast<int>(options));
    }
    return this;
}

static String prepare_pattern_for_onigmo(const String &pattern) {
    String new_pattern;
    auto length = pattern.length();
    size_t index = 0;

    auto next_char = [&length, &index, &pattern]() -> unsigned char {
        if (index >= length)
            return '\0';
        return pattern[index++];
    };

    EncodingObject *utf8 = nullptr;

    while (index < length) {
        auto c = next_char();

        switch (c) {

        case '\\': {
            c = next_char();
            switch (c) {

            case 'u': {
                c = next_char();

                if (!utf8)
                    utf8 = EncodingObject::get(Encoding::UTF_8);

                switch (c) {

                // Convert \u{dd} to \udddd
                case '{': {
                    c = next_char();
                    do {
                        long codepoint = 0;
                        while (isxdigit(c)) {
                            codepoint *= 16;
                            if (isdigit(c))
                                codepoint += c - '0';
                            else if (c >= 'a' && c <= 'f')
                                codepoint += c - 'a' + 10;
                            else if (c >= 'A' && c <= 'F')
                                codepoint += c - 'A' + 10;
                            else
                                break;
                            c = next_char();
                        }
                        if ((c == '}' || c == ' ') && utf8->in_encoding_codepoint_range(codepoint))
                            new_pattern.append(utf8->encode_codepoint(codepoint));
                        else
                            new_pattern.append_char(c);
                        while (c == ' ')
                            c = next_char();
                    } while (c != '}');
                    break;
                }

                default:
                    new_pattern.append_char('\\');
                    new_pattern.append_char('u');
                    new_pattern.append_char(c);
                }

                break;
            }

            default:
                new_pattern.append_char('\\');
                new_pattern.append_char(c);
            }

            break;
        }

        default:
            new_pattern.append_char(c);
        }
    }

    return new_pattern;
}

void RegexpObject::initialize(Env *env, const String &pattern, int options) {
    regex_t *regex;
    OnigErrorInfo einfo;
    m_pattern = pattern;

    auto tweaked_pattern = prepare_pattern_for_onigmo(pattern);

    UChar *pat = (UChar *)tweaked_pattern.c_str();
    // NATFIXME: Fully support character encodings in capture groups
    int result = onig_new(&regex, pat, pat + tweaked_pattern.length(),
        options, ONIG_ENCODING_UTF_8, ONIG_SYNTAX_DEFAULT, &einfo);
    if (result != ONIG_NORMAL) {
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result, &einfo);
        env->raise("RegexpError", "{}: /{}/", reinterpret_cast<const char *>(s), pattern);
    }
    m_regex = regex;
    m_options = options;
}

Value RegexpObject::inspect(Env *env) {
    if (!is_initialized())
        return KernelModule::inspect(env, this);
    StringObject *out = new StringObject { "/" };
    auto str = pattern();
    size_t len = str.length();
    regexp_stringify(str, 0, len, out);
    out->append_char('/');
    if (options() & RegexOpts::MultiLine) out->append_char('m');
    if (options() & RegexOpts::IgnoreCase) out->append_char('i');
    if (options() & RegexOpts::Extended) out->append_char('x');
    if (options() & RegexOpts::NoEncoding) out->append_char('n');
    return out;
}

Value RegexpObject::encoding(Env *env) {
    // NATFIXME: Implement support for other encodings.
    return EncodingObject::get(Encoding::US_ASCII);
}

Value RegexpObject::eqtilde(Env *env, Value other) {
    Value result = match(env, other);
    if (result->is_nil()) {
        return result;
    } else {
        MatchDataObject *matchdata = result->as_match_data();
        assert(matchdata->size() > 0);
        return IntegerObject::from_ssize_t(env, matchdata->beg_char_index(env, 0));
    }
}

Value RegexpObject::match(Env *env, Value other, Value start, Block *block) {
    assert_initialized(env);

    Env *caller_env = env->caller();
    if (other->is_nil()) {
        caller_env->clear_match();
        return NilObject::the();
    }

    if (other->is_symbol())
        other = other->as_symbol()->to_s(env);
    other->assert_type(env, Object::Type::String, "String");
    StringObject *str_obj = other->as_string();

    nat_int_t start_char_index = 0;
    nat_int_t start_byte_index = 0;
    if (start != nullptr) {
        start_byte_index = start->as_integer_or_raise(env)->to_nat_int_t();

        if (start_byte_index < 0) {
            size_t byte_index = str_obj->bytesize();
            ssize_t char_index = 0;
            TM::StringView view;
            do {
                view = str_obj->prev_char(&byte_index);
                char_index--;
            } while (byte_index != 0 && start_byte_index < char_index);
            start_byte_index = byte_index;
        } else {
            size_t byte_index = 0;
            ssize_t char_index = 0;
            TM::StringView view;
            while (start_byte_index > char_index) {
                view = str_obj->next_char(&byte_index);
                char_index++;
                if (view.is_empty()) break;
            }
            start_byte_index = byte_index;
        }
    }

    auto result = match_at_byte_offset(env, str_obj, start_byte_index);

    if (block && !result->is_nil()) {
        Value args[] = { result };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, Args(1, args), nullptr);
    }

    return result;
}

Value RegexpObject::match_at_byte_offset(Env *env, StringObject *str, size_t byte_index) {
    assert_initialized(env);

    Env *caller_env = env->caller();

    OnigRegion *region = onig_region_new();
    int result = search(str->string(), byte_index, region, ONIG_OPTION_NONE);

    if (result >= 0) {
        auto match = new MatchDataObject { region, str, this };
        caller_env->set_last_match(match);

        return match;
    } else if (result == ONIG_MISMATCH) {
        caller_env->clear_match();
        onig_region_free(region, true);
        return NilObject::the();
    } else {
        caller_env->clear_match();
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        env->raise("RuntimeError", (char *)s);
    }
}

bool RegexpObject::has_match(Env *env, Value other, Value start) {
    assert_initialized(env);
    if (other->is_nil())
        return false;
    if (other->is_symbol())
        other = other->as_symbol()->to_s(env);

    other->assert_type(env, Object::Type::String, "String");
    StringObject *str_obj = other->as_string();

    nat_int_t start_index = 0;
    if (start != nullptr) {
        if (start.is_fast_integer()) {
            start_index = start.get_fast_integer();
        } else if (start->is_integer()) {
            start_index = start->as_integer()->to_nat_int_t();
        }
    }
    if (start_index < 0) {
        start_index += str_obj->length();
    }

    OnigRegion *region = onig_region_new();
    int result = search(str_obj->string(), start_index, region, ONIG_OPTION_NONE);

    if (result >= 0) {
        return true;
    } else if (result == ONIG_MISMATCH) {
        onig_region_free(region, true);
        return false;
    } else {
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        env->raise("RuntimeError", (char *)s);
    }
}

Value RegexpObject::named_captures(Env *env) const {
    if (!m_regex)
        return new HashObject {};

    auto named_captures = new HashObject {};
    named_captures_data data { env, named_captures };
    onig_foreach_name(
        m_regex,
        [](const UChar *name, const UChar *name_end, int groups_size, int *groups, regex_t *regex, void *data) -> int {
            auto env = (static_cast<named_captures_data *>(data))->env;
            auto named_captures = (static_cast<named_captures_data *>(data))->named_captures;
            const size_t length = name_end - name;
            auto key = new StringObject { reinterpret_cast<const char *>(name), length, onig_encoding_to_ruby_encoding(regex->enc) };
            auto values = new ArrayObject { static_cast<size_t>(groups_size) };
            for (size_t i = 0; i < static_cast<size_t>(groups_size); i++)
                values->push(new IntegerObject { groups[i] });
            named_captures->put(env, key, values);
            return 0;
        },
        &data);
    return named_captures;
}

Value RegexpObject::names() const {
    if (!m_regex)
        return new ArrayObject {};

    auto names = new ArrayObject { static_cast<size_t>(onig_number_of_names(m_regex)) };
    onig_foreach_name(
        m_regex,
        [](const UChar *name, const UChar *name_end, int, int *, regex_t *regex, void *data) -> int {
            auto names = static_cast<ArrayObject *>(data);
            const size_t length = name_end - name;
            names->push(new StringObject { reinterpret_cast<const char *>(name), length, onig_encoding_to_ruby_encoding(regex->enc) });
            return 0;
        },
        names);
    return names;
}

Value RegexpObject::source(Env *env) const {
    assert_initialized(env);
    return new StringObject { pattern() };
}

Value RegexpObject::to_s(Env *env) const {
    assert_initialized(env);
    StringObject *out = new StringObject { "(" };

    auto is_m = options() & RegexOpts::MultiLine;
    auto is_i = options() & RegexOpts::IgnoreCase;
    auto is_x = options() & RegexOpts::Extended;

    auto str = pattern();
    size_t len = str.length();
    size_t start = 0;

    auto maybe_parse_opts = [&]() {
        if (str[start] == '(' && (start + 1) < len && str[start + 1] == '?' && str[len - 1]) {
            /*
            if there is only a single group fully-enclosing the regex, then
            we won't need to wrap it all in another group to specify the options
            */
            bool active = true;
            size_t i;
            bool will_be_m = is_m;
            bool will_be_i = is_i;
            bool will_be_x = is_x;
            for (i = start + 2; i < len && str[i] != ':'; i++) {
                auto c = str[i];
                switch (c) {
                case 'm':
                    will_be_m = active;
                    break;
                case 'i':
                    will_be_i = active;
                    break;
                case 'x':
                    will_be_x = active;
                    break;
                case '-':
                    if (!active) // this means we've already encountered a '-' which is illegal, so we just to append_options;
                        return;
                    active = false;
                    break;
                default:
                    return;
                }
            }
            {
                size_t open_parentheses = 1;
                // check that the first group is the only top-level group
                for (size_t j = i; j < len; ++j) {
                    if (str[j] == ')')
                        open_parentheses--;
                    if (str[j] == '(')
                        open_parentheses++;
                    if (open_parentheses == 0 && j != (len - 1))
                        return;
                }
            }
            is_i = will_be_i;
            is_m = will_be_m;
            is_x = will_be_x;
            len--;
            start = i + 1;
        }
    };

    maybe_parse_opts();

    out->append_char('?');

    if (is_m) out->append_char('m');
    if (is_i) out->append_char('i');
    if (is_x) out->append_char('x');

    if (!(is_m && is_i && is_x)) out->append_char('-');

    if (!is_m) out->append_char('m');
    if (!is_i) out->append_char('i');
    if (!is_x) out->append_char('x');

    out->append_char(':');
    regexp_stringify(str, start, len, out);
    out->append_char(')');

    return out;
}

String RegexpObject::dbg_inspect() const {
    return String::format("/{}/", m_pattern);
}

}
