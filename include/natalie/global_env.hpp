#pragma once

#include <mutex>
#include <stdlib.h>

#include "natalie/encodings.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_variable_info.hpp"
#include "natalie/method_missing_reason.hpp"
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

    ClassObject *BasicObject() { return m_BasicObject; }
    void set_BasicObject(ClassObject *BasicObject) { m_BasicObject = BasicObject; }

    ClassObject *Binding() { return m_Binding; }
    void set_Binding(ClassObject *Binding) { m_Binding = Binding; }

    ClassObject *Class() { return m_Class; }
    void set_Class(ClassObject *Class) { m_Class = Class; }

    ClassObject *Complex() { return m_Complex; }
    void set_Complex(ClassObject *Complex) { m_Complex = Complex; }

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

    ClassObject *Rational() { return m_Rational; }
    void set_Rational(ClassObject *Rational) { m_Rational = Rational; }

    ClassObject *Regexp() { return m_Regexp; }
    void set_Regexp(ClassObject *Regexp) { m_Regexp = Regexp; }

    ClassObject *String() { return m_String; }
    void set_String(ClassObject *String) { m_String = String; }

    ClassObject *Symbol() { return m_Symbol; }
    void set_Symbol(ClassObject *Symbol) { m_Symbol = Symbol; }

    ClassObject *Time() { return m_Time; }
    void set_Time(ClassObject *Time) { m_Time = Time; }

    Natalie::Object *main_obj() { return m_main_obj; }
    void set_main_obj(Natalie::Object *main_obj) { m_main_obj = main_obj; }

    bool global_defined(Env *, SymbolObject *);
    Value global_get(Env *, SymbolObject *);
    Value global_set(Env *, SymbolObject *, Value, bool = false);
    Value global_alias(Env *, SymbolObject *, SymbolObject *);
    ArrayObject *global_list(Env *);
    void global_set_read_hook(Env *, SymbolObject *, bool, GlobalVariableInfo::read_hook_t read_hook);
    void global_set_write_hook(Env *, SymbolObject *, GlobalVariableInfo::write_hook_t);

    bool is_verbose() const { return m_verbose; }
    void set_verbose(const bool verbose) { m_verbose = verbose; }

    void set_main_env(Env *main_env) { m_main_env = main_env; }
    Env *main_env() { return m_main_env; }

    MethodMissingReason method_missing_reason() const { return m_method_missing_reason; }
    void set_method_missing_reason(MethodMissingReason reason) { m_method_missing_reason = reason; }

    bool instance_evaling() const { return m_instance_evaling; }
    void set_instance_evaling(bool instance_evaling) { m_instance_evaling = instance_evaling; }

    bool rescued() const { return m_rescued; }
    void set_rescued(bool rescued) { m_rescued = rescued; }

    bool has_file(SymbolObject *name) const { return m_files.get(name); }
    void add_file(Env *env, SymbolObject *name);

    friend class SymbolObject;

    virtual void visit_children(Visitor &visitor) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<GlobalEnv %p>", this);
    }

private:
    GlobalEnv() { }

    inline static GlobalEnv *s_instance = nullptr;

    TM::Hashmap<SymbolObject *, GlobalVariableInfo *> m_global_variables {};

    ClassObject *m_Array { nullptr };
    ClassObject *m_BasicObject { nullptr };
    ClassObject *m_Binding { nullptr };
    ClassObject *m_Class { nullptr };
    ClassObject *m_Complex { nullptr };
    ClassObject *m_Float { nullptr };
    ClassObject *m_Hash { nullptr };
    ClassObject *m_Integer { nullptr };
    ClassObject *m_Module { nullptr };
    ClassObject *m_Object { nullptr };
    ClassObject *m_Random { nullptr };
    ClassObject *m_Rational { nullptr };
    ClassObject *m_Regexp { nullptr };
    ClassObject *m_String { nullptr };
    ClassObject *m_Symbol { nullptr };
    ClassObject *m_Time { nullptr };
    Natalie::Object *m_main_obj { nullptr };

    ClassObject *m_Encodings[EncodingCount];

    Env *m_main_env { nullptr };
    MethodMissingReason m_method_missing_reason { MethodMissingReason::Undefined };
    bool m_instance_evaling { false };
    bool m_rescued { false };
    bool m_verbose { false };

    TM::Hashmap<SymbolObject *> m_files {};
};
}
