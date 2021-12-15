#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {

extern "C" {
#include "onigmo.h"
}

class GlobalEnv : public Cell {
public:
    static GlobalEnv *the() {
        if (s_instance)
            return s_instance;
        s_instance = new GlobalEnv();
        return s_instance;
    }

    virtual ~GlobalEnv() override { }

    ClassObject *Array() { return m_Array; }
    void set_Array(ClassObject *Array) { m_Array = Array; }

    ClassObject *Binding() { return m_Binding; }
    void set_Binding(ClassObject *Binding) { m_Binding = Binding; }

    ClassObject *Class() { return m_Class; }
    void set_Class(ClassObject *Class) { m_Class = Class; }

    ClassObject *Float() { return m_Float; }
    void set_Float(ClassObject *Float) { m_Float = Float; }

    ClassObject *Hash() { return m_Hash; }
    void set_Hash(ClassObject *Hash) { m_Hash = Hash; }

    ClassObject *Integer() { return m_Integer; }
    void set_Integer(ClassObject *Integer) { m_Integer = Integer; }

    ClassObject *Module() { return m_Module; }
    void set_Module(ClassObject *Module) { m_Module = Module; }

    ClassObject *Object() { return m_Object; }
    void set_Object(ClassObject *Object) { m_Object = Object; }

    ClassObject *Random() { return m_Random; }
    void set_Random(ClassObject *Random) { m_Random = Random; }

    ClassObject *Regexp() { return m_Regexp; }
    void set_Regexp(ClassObject *Regexp) { m_Regexp = Regexp; }

    ClassObject *String() { return m_String; }
    void set_String(ClassObject *String) { m_String = String; }

    ClassObject *Symbol() { return m_Symbol; }
    void set_Symbol(ClassObject *Symbol) { m_Symbol = Symbol; }

    Natalie::Object *main_obj() { return m_main_obj; }
    void set_main_obj(Natalie::Object *main_obj) { m_main_obj = main_obj; }

    ValuePtr global_get(Env *, SymbolObject *);
    ValuePtr global_set(Env *, SymbolObject *, ValuePtr);

    void set_main_env(Env *main_env) { m_main_env = main_env; }
    Env *main_env() { return m_main_env; }

    friend class SymbolObject;

    virtual void visit_children(Visitor &visitor) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<GlobalEnv %p>", this);
    }

private:
    GlobalEnv() { }

    inline static GlobalEnv *s_instance = nullptr;

    TM::Hashmap<SymbolObject *, Natalie::Object *> m_globals {};

    ClassObject *m_Array { nullptr };
    ClassObject *m_Binding { nullptr };
    ClassObject *m_Class { nullptr };
    ClassObject *m_Float { nullptr };
    ClassObject *m_Hash { nullptr };
    ClassObject *m_Integer { nullptr };
    ClassObject *m_Module { nullptr };
    ClassObject *m_Object { nullptr };
    ClassObject *m_Random { nullptr };
    ClassObject *m_Regexp { nullptr };
    ClassObject *m_String { nullptr };
    ClassObject *m_Symbol { nullptr };
    Natalie::Object *m_main_obj { nullptr };

    Env *m_main_env { nullptr };
};
}
