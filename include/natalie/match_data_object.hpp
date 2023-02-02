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
        : Object { Object::Type::MatchData, GlobalEnv::the()->Object()->const_fetch("MatchData"_s)->as_class() } { }

    MatchDataObject(ClassObject *klass)
        : Object { Object::Type::MatchData, klass } { }

    MatchDataObject(OnigRegion *region, StringObject *string)
        : Object { Object::Type::MatchData, GlobalEnv::the()->Object()->const_fetch("MatchData"_s)->as_class() }
        , m_region { region }
        , m_string { string } { }

    virtual ~MatchDataObject() override {
        onig_region_free(m_region, true);
    }

    const StringObject *string() { return m_string; }

    size_t size() { return m_region->num_regs; }

    ssize_t index(size_t);
    ssize_t ending(size_t);

    Value array(int);
    Value group(int);
    Value offset(Env *, Value);

    Value captures(Env *);
    Value inspect(Env *);
    Value match(Env *, Value);
    Value match_length(Env *, Value);
    Value to_a(Env *);
    Value to_s(Env *);
    Value ref(Env *, Value);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MatchDataObject %p>", this);
    }

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        visitor.visit(m_string);
    }

    /**
     * If the underlying string that this MatchDataObject references is going to
     * be mutated in place, then we need to dup the source string so that we are
     * not impacted by those changes.
     */
    void dup_string(Env *env) {
        m_string = m_string->dup(env)->as_string();
    }

private:
    OnigRegion *m_region { nullptr };
    StringObject *m_string { nullptr };
};
}
