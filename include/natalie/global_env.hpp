#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/hashmap.hpp"

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

    ClassValue *Array() { return m_Array; }
    void set_Array(ClassValue *Array) { m_Array = Array; }

    ClassValue *Binding() { return m_Binding; }
    void set_Binding(ClassValue *Binding) { m_Binding = Binding; }

    ClassValue *Class() { return m_Class; }
    void set_Class(ClassValue *Class) { m_Class = Class; }

    ClassValue *Float() { return m_Float; }
    void set_Float(ClassValue *Float) { m_Float = Float; }

    ClassValue *Hash() { return m_Hash; }
    void set_Hash(ClassValue *Hash) { m_Hash = Hash; }

    ClassValue *Integer() { return m_Integer; }
    void set_Integer(ClassValue *Integer) { m_Integer = Integer; }

    ClassValue *Module() { return m_Module; }
    void set_Module(ClassValue *Module) { m_Module = Module; }

    ClassValue *Object() { return m_Object; }
    void set_Object(ClassValue *Object) { m_Object = Object; }

    ClassValue *Regexp() { return m_Regexp; }
    void set_Regexp(ClassValue *Regexp) { m_Regexp = Regexp; }

    ClassValue *String() { return m_String; }
    void set_String(ClassValue *String) { m_String = String; }

    ClassValue *Symbol() { return m_Symbol; }
    void set_Symbol(ClassValue *Symbol) { m_Symbol = Symbol; }

    Value *main_obj() { return m_main_obj; }
    void set_main_obj(Value *main_obj) { m_main_obj = main_obj; }

    ValuePtr global_get(Env *, SymbolValue *);
    ValuePtr global_set(Env *, SymbolValue *, ValuePtr);

    void set_main_env(Env *main_env) { m_main_env = main_env; }
    Env *main_env() { return m_main_env; }

    friend class SymbolValue;

    virtual void visit_children(Visitor &visitor) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<GlobalEnv %p>", this);
    }

private:
    GlobalEnv() { }

    inline static GlobalEnv *s_instance = nullptr;

    Hashmap<SymbolValue *, Value *> m_globals {};

    ClassValue *m_Array { nullptr };
    ClassValue *m_Binding { nullptr };
    ClassValue *m_Class { nullptr };
    ClassValue *m_Float { nullptr };
    ClassValue *m_Hash { nullptr };
    ClassValue *m_Integer { nullptr };
    ClassValue *m_Module { nullptr };
    ClassValue *m_Object { nullptr };
    ClassValue *m_Regexp { nullptr };
    ClassValue *m_String { nullptr };
    ClassValue *m_Symbol { nullptr };
    Value *m_main_obj { nullptr };

    Env *m_main_env { nullptr };
};
}
