#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, env->Module() } { }

ModuleValue::ModuleValue(Env *env, const char *name)
    : ModuleValue { env } {
    this->set_class_name(name);
}

ModuleValue::ModuleValue(Env *env, Type type, ClassValue *klass)
    : Value { type, klass }
    , m_env { Env::new_detatched_env(env) } { }

ValuePtr ModuleValue::extend(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        extend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::extend_once(Env *env, ModuleValue *module) {
    singleton_class(env)->include_once(env, module);
}

ValuePtr ModuleValue::include(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        include_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::include_once(Env *env, ModuleValue *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
    }
    m_included_modules.push(module);
}

ValuePtr ModuleValue::prepend(Env *env, size_t argc, ValuePtr *args) {
    for (int i = argc - 1; i >= 0; i--) {
        prepend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::prepend_once(Env *env, ModuleValue *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
    }
    m_included_modules.push_front(module);
}

ValuePtr ModuleValue::const_get(Env *env, SymbolValue *name) {
    return m_constants.get(env, name);
}

ValuePtr ModuleValue::const_fetch(Env *env, SymbolValue *name) {
    ValuePtr value = const_get(env, name);
    if (!value) {
        printf("Constant %s is missing!\n", name->c_str());
        abort();
    }
    return value;
}

ValuePtr ModuleValue::const_find(Env *env, SymbolValue *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    ModuleValue *search_parent;
    ValuePtr val;

    if (search_mode == ConstLookupSearchMode::NotStrict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = this;
        while (!(val = search_parent->const_get(env, name)) && search_parent->owner() && search_parent->owner() != env->Object()) {
            search_parent = search_parent->owner();
        }
        if (val) return val;
    }

    // search in superclass hierarchy
    search_parent = this;
    do {
        val = search_parent->const_get(env, name);
        if (val) return val;
        search_parent = search_parent->m_superclass;
    } while (search_parent);

    if (this != env->Object() && search_mode == ConstLookupSearchMode::NotStrict) {
        // lastly, search on the global, i.e. Object namespace
        val = env->Object()->const_get(env, name);
        if (val) return val;
    }

    if (failure_mode == ConstLookupFailureMode::Null) return nullptr;

    if (search_mode == ConstLookupSearchMode::Strict) {
        env->raise("NameError", "uninitialized constant %s::%s", this->inspect_str(env), name->c_str());
    } else {
        env->raise("NameError", "uninitialized constant %s", name->c_str());
    }
}

ValuePtr ModuleValue::const_set(Env *env, SymbolValue *name, ValuePtr val) {
    m_constants.put(env, name, val.value());
    if (val->is_module() && !val->owner()) {
        val->set_owner(this);
        if (val->singleton_class()) val->singleton_class()->set_owner(this);
    }
    return val;
}

void ModuleValue::alias(Env *env, SymbolValue *new_name, SymbolValue *old_name) {
    Method *method = find_method(env, old_name);
    if (!method) {
        env->raise("NameError", "undefined method `%s' for `%v'", old_name->c_str(), this);
    }
    m_methods.put(env, new_name, new Method { *method });
}

ValuePtr ModuleValue::eval_body(Env *env, ValuePtr (*fn)(Env *, ValuePtr)) {
    Env body_env = Env { env };
    body_env.set_caller(env);
    ValuePtr result = fn(&body_env, this);
    body_env.clear_caller();
    return result;
}

ValuePtr ModuleValue::cvar_get_or_null(Env *env, SymbolValue *name) {
    if (!name->is_cvar_name())
        env->raise("NameError", "`%s' is not allowed as a class variable name", name->c_str());

    ModuleValue *module = this;
    ValuePtr val = nullptr;
    while (module) {
        val = module->m_class_vars.get(env, name);
        if (val)
            return val;
        module = module->m_superclass;
    }
    return nullptr;
}

ValuePtr ModuleValue::cvar_set(Env *env, SymbolValue *name, ValuePtr val) {
    if (!name->is_cvar_name())
        env->raise("NameError", "`%s' is not allowed as a class variable name", name->c_str());

    ModuleValue *current = this;

    ValuePtr exists = nullptr;
    while (current) {
        exists = current->m_class_vars.get(env, name);
        if (exists) {
            current->m_class_vars.put(env, name, val.value());
            return val;
        }
        current = current->m_superclass;
    }
    m_class_vars.put(env, name, val.value());
    return val;
}

SymbolValue *ModuleValue::define_method(Env *env, SymbolValue *name, MethodFnPtr fn, int arity) {
    Method *method = new Method { name->c_str(), this, fn, arity, m_method_visibility };
    m_methods.put(env, name, method);
    return name;
}

SymbolValue *ModuleValue::define_method(Env *env, SymbolValue *name, Block *block) {
    Method *method = new Method { name->c_str(), this, block, m_method_visibility };
    m_methods.put(env, name, method);
    return name;
}

SymbolValue *ModuleValue::undefine_method(Env *env, SymbolValue *name) {
    return define_method(env, name, nullptr);
}

// supply an empty array and it will be populated with the method names as symbols
void ModuleValue::methods(Env *env, ArrayValue *array) {
    for (auto pair : m_methods) {
        array->push(pair.first);
    }
    for (ModuleValue *module : m_included_modules) {
        for (auto pair : module->m_methods) {
            array->push(pair.first);
        }
    }
    if (m_superclass) {
        return m_superclass->methods(env, array);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
Method *ModuleValue::find_method(Env *env, SymbolValue *method_name, ModuleValue **matching_class_or_module) {
    Method *method;
    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = m_methods.get(env, method_name);
        if (method) {
            if (matching_class_or_module) *matching_class_or_module = m_klass;
            return method;
        }
    }

    for (ModuleValue *module : m_included_modules) {
        method = module->m_methods.get(env, method_name);
        if (method) {
            if (matching_class_or_module) *matching_class_or_module = module;
            return method;
        }
    }

    if (m_superclass) {
        return m_superclass->find_method(env, method_name, matching_class_or_module);
    } else {
        return nullptr;
    }
}

Method *ModuleValue::find_method(Env *env, const char *method_name, ModuleValue **matching_class_or_module) {
    return find_method(env, SymbolValue::intern(env, method_name), matching_class_or_module);
}

// TODO: remove this
ValuePtr ModuleValue::call_method(Env *env, ValuePtr instance_class, SymbolValue *method_name, ValuePtr self, size_t argc, ValuePtr *args, Block *block, MethodVisibility visibility_at_least) {
    Method *method = find_method(env, method_name);
    if (method && !method->is_undefined()) {
        if (method->visibility() >= visibility_at_least) {
            return method->call(env, self, argc, args, block);
        } else {
            env->raise("NoMethodError", "private method `%s' called for %s", method_name->c_str(), self->inspect_str(env));
        }
    } else if (self->is_module()) {
        env->raise("NoMethodError", "undefined method `%s' for %s:%v", method_name->c_str(), self->as_module()->class_name(), instance_class.value());
    } else if (method_name == SymbolValue::intern(env, "inspect")) {
        env->raise("NoMethodError", "undefined method `inspect' for #<%s:0x%x>", self->klass()->class_name(), self->object_id());
    } else {
        env->raise("NoMethodError", "undefined method `%s' for %s", method_name->c_str(), self->inspect_str(env));
    }
}

// TODO: remove this
ValuePtr ModuleValue::call_method(Env *env, ValuePtr instance_class, const char *method_name, ValuePtr self, size_t argc, ValuePtr *args, Block *block, MethodVisibility visibility_at_least) {
    return call_method(env, instance_class, SymbolValue::intern(env, method_name), self, argc, args, block, visibility_at_least);
}

ArrayValue *ModuleValue::ancestors(Env *env) {
    ModuleValue *klass = this;
    ArrayValue *ancestors = new ArrayValue { env };
    do {
        if (klass->included_modules().is_empty()) {
            // note: if there are included modules, then they will include this klass
            ancestors->push(klass);
        }
        for (ModuleValue *m : klass->included_modules()) {
            ancestors->push(m);
        }
        klass = klass->superclass();
    } while (klass);
    return ancestors;
}

bool ModuleValue::is_method_defined(Env *env, ValuePtr name_value) {
    const char *name = name_value->identifier_str(env, Conversion::Strict);
    return !!find_method(env, name);
}

ValuePtr ModuleValue::inspect(Env *env) {
    if (m_class_name) {
        if (owner() && owner() != env->Object()) {
            return StringValue::format(env, "{}::{}", owner()->inspect_str(env), m_class_name);
        } else {
            return new StringValue { env, m_class_name };
        }
    } else if (is_class()) {
        return StringValue::format(env, "#<Class:{}>", pointer_id());
    } else if (is_module() && m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        return StringValue::format(env, "#<{}:{}>", klass()->inspect_str(env), pointer_id());
    }
}

ValuePtr ModuleValue::name(Env *env) {
    if (m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        return env->nil_obj();
    }
}

ValuePtr ModuleValue::attr_reader(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        ValuePtr name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "%s is not a symbol nor a string", name_obj->inspect_str(env));
        }
        Env block_env = Env::new_detatched_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_reader_block_fn, 0 };
        define_method(env, name_obj->as_string()->to_symbol(env), attr_block);
    }
    return env->nil_obj();
}

ValuePtr ModuleValue::attr_reader_block_fn(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    ValuePtr name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    assert(name_obj->is_string());
    StringValue *ivar_name = StringValue::format(env, "@{}", name_obj->as_string());
    return self->ivar_get(env, ivar_name->to_symbol(env));
}

ValuePtr ModuleValue::attr_writer(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        ValuePtr name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "%s is not a symbol nor a string", name_obj->inspect_str(env));
        }
        StringValue *method_name = new StringValue { env, name_obj->as_string()->c_str() };
        method_name->append(env, '=');
        Env block_env = Env::new_detatched_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_writer_block_fn, 0 };
        define_method(env, method_name->to_symbol(env), attr_block);
    }
    return env->nil_obj();
}

ValuePtr ModuleValue::attr_writer_block_fn(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    ValuePtr val = args[0];
    ValuePtr name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    assert(name_obj->is_string());
    StringValue *ivar_name = StringValue::format(env, "@{}", name_obj->as_string());
    self->ivar_set(env, ivar_name->to_symbol(env), val);
    return val;
}

ValuePtr ModuleValue::attr_accessor(Env *env, size_t argc, ValuePtr *args) {
    attr_reader(env, argc, args);
    attr_writer(env, argc, args);
    return env->nil_obj();
}

ValuePtr ModuleValue::included_modules(Env *env) {
    ArrayValue *modules = new ArrayValue { env };
    for (ModuleValue *m : included_modules()) {
        modules->push(m);
    }
    return modules;
}

bool ModuleValue::does_include_module(Env *env, ValuePtr module) {
    module->assert_type(env, Value::Type::Module, "Module");
    for (ModuleValue *m : included_modules()) {
        if (m == this) continue;
        if (module == m) return true;
    }
    return false;
}

ValuePtr ModuleValue::define_method(Env *env, ValuePtr name_value, Block *block) {
    auto name = name_value->to_symbol(env, Value::Conversion::Strict);
    if (!block) {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
    define_method(env, name, block);
    return name;
}

ValuePtr ModuleValue::module_eval(Env *env, Block *block) {
    if (!block) {
        env->raise("ArgumentError", "Natalie only supports module_eval with a block");
    }
    block->set_self(this);
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    return env->nil_obj();
}

ValuePtr ModuleValue::private_method(Env *env, ValuePtr method_name) {
    m_method_visibility = MethodVisibility::Private;
    return env->nil_obj();
}

ValuePtr ModuleValue::protected_method(Env *env, ValuePtr method_name) {
    m_method_visibility = MethodVisibility::Protected;
    return env->nil_obj();
}

ValuePtr ModuleValue::public_method(Env *env, ValuePtr method_name) {
    m_method_visibility = MethodVisibility::Public;
    return env->nil_obj();
}

bool ModuleValue::const_defined(Env *env, ValuePtr name_value) {
    auto name = name_value->to_symbol(env, Value::Conversion::NullAllowed);
    if (!name) {
        env->raise("TypeError", "no implicit conversion of %v to String", name_value.value());
    }
    return !!const_find(env, name, ConstLookupSearchMode::NotStrict, ConstLookupFailureMode::Null);
}

ValuePtr ModuleValue::alias_method(Env *env, ValuePtr new_name_value, ValuePtr old_name_value) {
    auto new_name = new_name_value->to_symbol(env, Value::Conversion::NullAllowed);
    if (!new_name) {
        env->raise("TypeError", "%s is not a symbol", new_name_value->inspect_str(env));
    }
    auto old_name = old_name_value->to_symbol(env, Value::Conversion::NullAllowed);
    if (!old_name) {
        env->raise("TypeError", "%s is not a symbol", old_name_value->inspect_str(env));
    }
    alias(env, new_name, old_name);
    return this;
}

}
