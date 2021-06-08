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

    FiberValue *main_fiber(Env *);

    FiberValue *current_fiber() { return m_current_fiber; }
    void set_current_fiber(FiberValue *fiber) { m_current_fiber = fiber; }
    void reset_current_fiber() { m_current_fiber = m_main_fiber; }

    size_t fiber_argc() { return m_fiber_args.argc; }
    ValuePtr *fiber_args() { return m_fiber_args.args; }
    void set_fiber_args(size_t argc, ValuePtr *args) {
        m_fiber_args.argc = argc;
        m_fiber_args.args = args;
    }

    ValuePtr global_get(Env *, SymbolValue *);
    ValuePtr global_set(Env *, SymbolValue *, ValuePtr);

    friend class SymbolValue;

    virtual void visit_children(Visitor &visitor) override final;

    virtual void gc_print() override {
        fprintf(stderr, "<GlobalEnv %p>", this);
    }

private:
    GlobalEnv() { }

    inline static GlobalEnv *s_instance = nullptr;

    Hashmap<SymbolValue *, Value *> m_globals {};

    ClassValue *m_Array { nullptr };
    ClassValue *m_Class { nullptr };
    ClassValue *m_Float { nullptr };
    ClassValue *m_Hash { nullptr };
    ClassValue *m_Integer { nullptr };
    ClassValue *m_Module { nullptr };
    ClassValue *m_Object { nullptr };
    ClassValue *m_Regexp { nullptr };
    ClassValue *m_String { nullptr };
    ClassValue *m_Symbol { nullptr };

    FiberValue *m_main_fiber { nullptr };
    FiberValue *m_current_fiber { nullptr };
    struct {
        size_t argc { 0 };
        ValuePtr *args { nullptr };
    } m_fiber_args;
};
}
