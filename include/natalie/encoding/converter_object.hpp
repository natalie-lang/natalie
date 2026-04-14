#pragma once

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

enum class EconvResult {
    InvalidByteSequence,
    UndefinedConversion,
    DestinationBufferFull,
    SourceBufferEmpty,
    Finished,
    AfterOutput,
    IncompleteInput,
};

class EncodingConverterObject : public Object {
public:
    EncodingConverterObject(ClassObject *klass)
        : Object { Object::Type::EncodingConverter, klass } { }

    Value initialize(Env *, Value, Value, Optional<Value> = {});
    Value convert(Env *, Value);
    Value finish(Env *);
    Value inspect(Env *) const;

    EncodingObject *source_encoding() const { return m_source_encoding; }
    EncodingObject *destination_encoding() const { return m_destination_encoding; }
    Value replacement(Env *) const;
    Value set_replacement(Env *, Value);

    Value primitive_convert(Env *, Args &&);
    Value primitive_errinfo(Env *) const;
    Value convpath(Env *) const;
    Value putback(Env *, Optional<Value> = {});
    Value last_error(Env *) const;
    Value insert_output(Env *, Value);

    static Value asciicompat_encoding(Env *, Value);
    static Value search_convpath(Env *, Args &&);

    virtual void visit_children(Visitor &visitor) const override final {
        Object::visit_children(visitor);
        visitor.visit(m_source_encoding);
        visitor.visit(m_destination_encoding);
        visitor.visit(m_replacement_str);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<EncodingConverterObject {h}>", this);
    }

private:
    EncodingObject *m_source_encoding { nullptr };
    EncodingObject *m_destination_encoding { nullptr };
    int m_flags { 0 };
    bool m_started { false };
    bool m_finished { false };

    StringObject *m_replacement_str { nullptr };

    // Internal buffer for streaming conversion
    String m_input_buffer {};

    // Error state
    EconvResult m_last_result { EconvResult::SourceBufferEmpty };
    String m_error_bytes {};
    String m_readagain_bytes {};

    // Core conversion engine
    EconvResult do_convert(
        const String &input, size_t *input_pos,
        String *output, size_t max_output,
        int convert_flags);

    void set_default_replacement();
    static Value result_to_symbol(EconvResult result);
};

}
