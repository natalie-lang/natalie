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
    MatchDataValue(Env *env)
        : Value { Value::Type::MatchData, env->Object()->const_fetch(SymbolValue::intern("MatchData"))->as_class() } { }

    MatchDataValue(Env *env, ClassValue *klass)
        : Value { Value::Type::MatchData, klass } { }

    MatchDataValue(Env *env, OnigRegion *region, StringValue *string)
        : Value { Value::Type::MatchData, env->Object()->const_fetch(SymbolValue::intern("MatchData"))->as_class() }
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

    virtual void gc_print() override {
        fprintf(stderr, "<MatchDataValue %p>", this);
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
