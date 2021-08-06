#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class MatchDataValue : public Value {
public:
    MatchDataValue()
        : Value { Value::Type::MatchData, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("MatchData"))->as_class() } { }

    MatchDataValue(ClassValue *klass)
        : Value { Value::Type::MatchData, klass } { }

    MatchDataValue(OnigRegion *region, StringValue *string)
        : Value { Value::Type::MatchData, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("MatchData"))->as_class() }
        , m_region { region }
        , m_string { string } { }

    virtual ~MatchDataValue() override {
        onig_region_free(m_region, true);
    }

    const StringValue *string() { return m_string; }

    size_t size() { return m_region->num_regs; }

    size_t index(size_t);

    ValuePtr group(Env *, size_t);

    ValuePtr to_s(Env *);
    ValuePtr ref(Env *, ValuePtr);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MatchDataValue %p>", this);
    }

    virtual void visit_children(Visitor &visitor) override final {
        Value::visit_children(visitor);
        visitor.visit(m_string);
    }

private:
    OnigRegion *m_region { nullptr };
    const StringValue *m_string { nullptr };
};
}
