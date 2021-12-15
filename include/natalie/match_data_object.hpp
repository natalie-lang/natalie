#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class MatchDataObject : public Object {
public:
    MatchDataObject()
        : Object { Object::Type::MatchData, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("MatchData"))->as_class() } { }

    MatchDataObject(ClassObject *klass)
        : Object { Object::Type::MatchData, klass } { }

    MatchDataObject(OnigRegion *region, StringObject *string)
        : Object { Object::Type::MatchData, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("MatchData"))->as_class() }
        , m_region { region }
        , m_string { string } { }

    virtual ~MatchDataObject() override {
        onig_region_free(m_region, true);
    }

    const StringObject *string() { return m_string; }

    size_t size() { return m_region->num_regs; }

    size_t index(size_t);

    ValuePtr group(Env *, size_t);

    ValuePtr to_s(Env *);
    ValuePtr ref(Env *, ValuePtr);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MatchDataObject %p>", this);
    }

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        visitor.visit(m_string);
    }

private:
    OnigRegion *m_region { nullptr };
    const StringObject *m_string { nullptr };
};
}
