#include "natalie.hpp"
#include "natalie/global_env.hpp"
#include "tm/owned_ptr.hpp"

namespace Natalie {

ModuleObject::ModuleObject()
    : ModuleObject { Object::Type::Module, GlobalEnv::the()->Module() } { }

ModuleObject::ModuleObject(const char *name)
    : ModuleObject {} {
    this->set_name(name);
}

ModuleObject::ModuleObject(Type type, ClassObject *klass)
    : Object { type, klass } { }

Value ModuleObject::initialize(Env *env, Block *block) {
    if (block) {
        Value self = this;
        block->set_self(self);
        Value args[] = { self };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value ModuleObject::include(Env *env, Args &&args) {
    for (int i = args.size() - 1; i >= 0; i--) {
        include_once(env, args[i].as_module());
    }
    return this;
}

void ModuleObject::include_once(Env *env, ModuleObject *module) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
        m_included_modules.push(module);
    } else {
        ssize_t this_index = -1;
        for (size_t i = 0; i < m_included_modules.size(); ++i) {
            if (m_included_modules[i] == this)
                this_index = i;
            if (m_included_modules[i] == module)
                return;
        }
        assert(this_index != -1);
        m_included_modules.insert(this_index + 1, module);
    }
    if (module->respond_to(env, "included"_s))
        module->send(env, "included"_s, { this });
}

Value ModuleObject::prepend(Env *env, Args &&args) {
    for (int i = args.size() - 1; i >= 0; i--) {
        prepend_once(env, args[i].as_module());
    }
    return this;
}

void ModuleObject::prepend_once(Env *env, ModuleObject *module) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (m_included_modules.is_empty()) {
        m_included_modules.push(module);
        m_included_modules.push(this);
    } else {
        for (auto m : m_included_modules) {
            if (m == module)
                return;
        }
        m_included_modules.push_front(module);
    }
}

Value ModuleObject::extend_object(Env *env, Value obj) {
    obj.assert_not_frozen(env);
    obj.singleton_class(env)->include_once(env, this);
    return obj;
}

Optional<Value> ModuleObject::const_get(SymbolObject *name) const {
    auto constant = m_constants.get(name);
    if (!constant)
        return {};
    return constant->value();
}

Value ModuleObject::const_get(Env *env, Value name, Optional<Value> inherited) {
    auto symbol = name.to_symbol(env, Value::Conversion::Strict);
    auto constant = const_get(symbol);
    if (!constant) {
        if (inherited && inherited.value().is_falsey())
            env->raise("NameError", "uninitialized constant {}", symbol->string());
        return send(env, "const_missing"_s, { name });
    }
    return constant.value();
}

Value ModuleObject::const_fetch(SymbolObject *name) const {
    auto constant = const_get(name);
    if (!constant) {
        TM::String::format("Constant {} is missing!\n", name->string()).print();
        abort();
    }
    return constant.value();
}

Constant *ModuleObject::find_constant(Env *env, SymbolObject *name, ModuleObject **found_in_module, ConstLookupSearchMode search_mode) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ModuleObject *search_parent = nullptr;
    Constant *constant = nullptr;

    auto valid_search_module = [&](ModuleObject *module) {
        return module && module != this && module != GlobalEnv::the()->Object() && module != GlobalEnv::the()->BasicObject();
    };

    auto check_valid
        = [&](Constant *constant) {
              if (search_mode == ConstLookupSearchMode::Strict && constant->is_private()) {
                  if (search_parent && search_parent != GlobalEnv::the()->Object())
                      env->raise_name_error(name, "private constant {}::{} referenced", search_parent->inspect_module(), name->string());
                  else
                      env->raise_name_error(name, "private constant ::{} referenced", name->string());
              }
              if (constant->is_deprecated()) {
                  if (search_parent && search_parent != GlobalEnv::the()->Object())
                      env->deprecation_warn("constant {}::{} is deprecated", search_parent->inspect_module(), name->string());
                  else
                      env->deprecation_warn("constant ::{} is deprecated", name->string());
              }
          };

    constant = m_constants.get(name);
    if (constant) {
        search_parent = this;
        if (found_in_module) *found_in_module = search_parent;
        check_valid(constant);
        return constant;
    }

    if (search_mode == ConstLookupSearchMode::NotStrict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = this;
        ModuleObject *found = nullptr;
        do {
            search_parent = search_parent->owner();
            if (!valid_search_module(search_parent))
                break;
            constant = search_parent->find_constant(env, name, &found, search_mode);
        } while (!constant);
        if (constant) {
            search_parent = found;
            if (found_in_module) *found_in_module = search_parent;
            check_valid(constant);
            return constant;
        }
    }

    // search included modules
    Vector<ModuleObject *> modules_to_search;
    for (ModuleObject *module : m_included_modules) {
        if (module != this)
            modules_to_search.push(module);
    }
    for (size_t i = 0; i < modules_to_search.size(); ++i) {
        auto search_parent = modules_to_search.at(i);
        constant = search_parent->m_constants.get(name);
        if (constant) {
            if (found_in_module) *found_in_module = search_parent;
            break;
        }
        for (ModuleObject *m : search_parent->m_included_modules) {
            if (m != search_parent && m != this)
                modules_to_search.push(m);
        }
    }

    if (constant) {
        check_valid(constant);
        return constant;
    }

    if (search_mode != ConstLookupSearchMode::StrictPrivate) {
        // search in superclass hierarchy
        search_parent = this;
        ModuleObject *found = nullptr;
        do {
            search_parent = search_parent->m_superclass;
            if (!valid_search_module(search_parent))
                break;
            constant = search_parent->find_constant(env, name, &found, search_mode);
        } while (!constant);

        if (constant) {
            search_parent = found;
            if (found_in_module) *found_in_module = search_parent;
            check_valid(constant);
            return constant;
        }
    }

    if (this != GlobalEnv::the()->Object() && search_mode == ConstLookupSearchMode::NotStrict) {
        // lastly, search on the global, i.e. Object namespace
        search_parent = GlobalEnv::the()->Object();
        ModuleObject *found = nullptr;
        constant = search_parent->find_constant(env, name, &found, search_mode);
        if (found_in_module) *found_in_module = found;
    }

    if (constant) check_valid(constant);

    return constant;
}

Value ModuleObject::is_autoload(Env *env, Value name) const {
    auto name_sym = name.to_symbol(env, Value::Conversion::Strict);
    auto constant = m_constants.get(name_sym);
    if (constant && constant->needs_load()) {
        auto path = constant->autoload_path();
        assert(path);
        return path;
    }
    return Value::nil();
}

Optional<Value> ModuleObject::const_find_with_autoload(Env *env, Value self, SymbolObject *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    ModuleObject *module = nullptr;
    auto constant = find_constant(env, name, &module, search_mode);

    if (!constant)
        return handle_missing_constant(env, name, failure_mode);

    if (constant->needs_load()) {
        assert(module);
        module->remove_const(name);
        constant->autoload(env, self);
    }

    return const_find(env, name, search_mode, failure_mode);
}

Optional<Value> ModuleObject::const_find(Env *env, SymbolObject *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    auto constant = find_constant(env, name, nullptr, search_mode);

    if (!constant)
        return handle_missing_constant(env, name, failure_mode);

    return constant->value();
}

Optional<Value> ModuleObject::handle_missing_constant(Env *env, Value name, ConstLookupFailureMode failure_mode) {
    if (failure_mode == ConstLookupFailureMode::None)
        return Optional<Value>();

    if (failure_mode == ConstLookupFailureMode::Raise) {
        auto name_str = name.to_s(env);
        if (this == GlobalEnv::the()->Object())
            env->raise_name_error(name_str, "uninitialized constant {}", name_str->string());
        env->raise_name_error(name_str, "uninitialized constant {}::{}", inspect_module(), name_str->string());
    }

    return send(env, "const_missing"_s, { name });
}

ClassObject *ModuleObject::as_class() { return static_cast<ClassObject *>(this); }

Value ModuleObject::const_set(SymbolObject *name, Value val) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_constants.put(name, new Constant { name, val });
    if (val.is_module()) {
        auto module = val.as_module();
        if (!module->owner()) {
            module->m_owner = this;
            if (module->singleton_class())
                module->singleton_class()->m_owner = this;
        }

        if (!val.as_module()->name()) {
            auto module_name = name->string();
            val.as_module()->set_name(module_name);

            auto singleton_class = val->singleton_class();
            while (singleton_class) {
                auto class_name = String::format("#<Class:{}>", module_name);
                singleton_class->set_name(class_name);
                singleton_class = singleton_class->singleton_class();
            }
        }
    }
    return val;
}

Value ModuleObject::const_set(SymbolObject *name, MethodFnPtr autoload_fn, StringObject *autoload_path) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_constants.put(name, new Constant { name, autoload_fn, autoload_path });
    return Value::nil();
}

Value ModuleObject::const_set(Env *env, Value name, Value val) {
    auto name_as_sym = name.to_symbol(env, Value::Conversion::Strict);
    if (!name_as_sym->is_constant_name())
        env->raise_name_error(name_as_sym, "wrong constant name {}", name_as_sym->string());
    return const_set(name_as_sym, val);
}

void ModuleObject::remove_const(SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_constants.remove(name);
}

Value ModuleObject::remove_const(Env *env, Value name) {
    auto name_as_sym = name.to_symbol(env, Value::Conversion::Strict);
    if (!name_as_sym->is_constant_name())
        env->raise_name_error(name_as_sym, "wrong constant name {}", name_as_sym->string());
    auto constant = m_constants.get(name_as_sym);
    if (!constant)
        env->raise("NameError", "constant {} not defined", name_as_sym->string());
    remove_const(name_as_sym);
    return constant->value().value();
}

Value ModuleObject::constants(Env *env, Optional<Value> inherit) const {
    auto ary = new ArrayObject;
    for (auto pair : m_constants)
        ary->push(pair.first);
    if (!inherit || inherit.value().is_truthy()) {
        for (ModuleObject *module : m_included_modules) {
            if (module != this) {
                ary->concat(*module->constants(env, inherit).as_array());
            }
        }
    }
    return ary;
}

Value ModuleObject::const_missing(Env *env, Value name) {
    return handle_missing_constant(env, name, ConstLookupFailureMode::Raise).value();
}

void ModuleObject::make_method_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    auto method_info = find_method(env, old_name);
    assert_method_defined(env, old_name, method_info);

    auto old_method = method_info.method();
    auto new_method = Method::from_other(new_name->string(), old_method);
    define_method(env, new_name, new_method, method_info.visibility());
};

void ModuleObject::method_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    if (GlobalEnv::the()->instance_evaling()) {
        singleton_class(env, this)->make_method_alias(env, new_name, old_name);
        return;
    }
    make_method_alias(env, new_name, old_name);
}

Value ModuleObject::eval_body(Env *env, Value (*fn)(Env *, Value)) {
    Env body_env {};
    body_env.set_caller(env);
    body_env.set_module(this);
    Value result = fn(&body_env, this);
    m_method_visibility = MethodVisibility::Public;
    m_module_function = false;
    return result;
}

Optional<Value> ModuleObject::cvar_get_maybe(Env *env, SymbolObject *name) {
    if (!name->is_cvar_name())
        env->raise_name_error(name, "`{}' is not allowed as a class variable name", name->string());

    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ModuleObject *module = this;
    Optional<Value> val;
    while (module) {
        val = module->m_class_vars.get(name, env);
        if (val)
            return val;
        module = module->m_superclass;
    }

    for (auto *m : m_included_modules) {
        val = m->m_class_vars.get(name, env);
        if (val)
            return val;
    }

    if (singleton_class()) {
        val = singleton_class()->m_class_vars.get(name, env);
        if (val)
            return val;
    }

    return {};
}

Value ModuleObject::cvar_set(Env *env, SymbolObject *name, Value val) {
    auto set_cvar_in = [env, name, &val](ModuleObject *module) -> Value {
        if (!name->is_cvar_name())
            env->raise_name_error(name, "`{}' is not allowed as a class variable name", name->string());

        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

        ModuleObject *current = module;

        Optional<Value> exists;
        while (current) {
            exists = current->m_class_vars.get(name, env);
            if (exists) {
                current->m_class_vars.put(name, val, env);
                return val;
            }
            current = current->m_superclass;
        }
        module->m_class_vars.put(name, val, env);
        return val;
    };

    if (GlobalEnv::the()->instance_evaling()) {
        // Set class variable in block definition scope
        auto context = GlobalEnv::the()->current_instance_eval_context();
        if (context.block_original_self.is_module()) {
            return set_cvar_in(context.block_original_self.as_module());
        } else {
            return set_cvar_in(context.block_original_self.klass());
        }
    }

    return set_cvar_in(this);
}

bool ModuleObject::class_variable_defined(Env *env, Value name) {
    auto *name_sym = name.to_symbol(env, Value::Conversion::Strict);

    return cvar_get_maybe(env, name_sym).present();
}

Value ModuleObject::class_variable_get(Env *env, Value name) {
    auto *name_sym = name.to_symbol(env, Value::Conversion::Strict);

    auto val = cvar_get_maybe(env, name_sym);
    if (!val) {
        env->raise_name_error(name_sym, "uninitialized class variable {} in {}", name_sym->string(), inspect_module());
    }
    return val.value();
}

Value ModuleObject::class_variable_set(Env *env, Value name, Value value) {
    assert_not_frozen(env);

    return cvar_set(env, name.to_symbol(env, Value::Conversion::Strict), value);
}

ArrayObject *ModuleObject::class_variables(Optional<Value> inherit) const {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto result = new ArrayObject {};
    for (auto [cvar, _] : m_class_vars)
        result->push(cvar);
    if (singleton_class()) {
        for (auto [cvar, _] : singleton_class()->m_class_vars)
            result->push(cvar);
    }
    if (inherit && inherit.value().is_truthy() && m_superclass)
        result->concat(*m_superclass->class_variables(inherit));
    return result;
}

Value ModuleObject::remove_class_variable(Env *env, Value name) {
    assert_not_frozen(env);
    auto *name_sym = name.to_symbol(env, Value::Conversion::Strict);

    if (!name_sym->is_cvar_name())
        env->raise_name_error(name_sym, "`{}' is not allowed as a class variable name", name_sym->string());

    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto val = cvar_get_maybe(env, name_sym);
    if (!val)
        env->raise_name_error(name_sym, "uninitialized class variable {} in {}", name_sym->string(), inspect_module());

    m_class_vars.remove(name_sym);

    return val.value();
}

SymbolObject *ModuleObject::define_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity) {
    assert_not_frozen(env, this);
    Method *method = new Method { name->string(), this, fn, arity, env->file(), env->line() };
    auto visibility = m_method_visibility;
    if (name == "initialize"_s)
        visibility = MethodVisibility::Private;
    define_method(env, name, method, visibility);
    if (m_module_function)
        Object::define_singleton_method(env, this, name, fn, arity);
    return name;
}

SymbolObject *ModuleObject::define_method(Env *env, SymbolObject *name, Block *block) {
    assert_not_frozen(env, this);
    Method *method = new Method { name->string(), this, block };
    define_method(env, name, method, m_method_visibility);
    if (m_module_function)
        Object::define_singleton_method(env, this, name, block);
    return name;
}

SymbolObject *ModuleObject::undefine_method(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    GlobalEnv::the()->increment_method_cache_version();
    m_methods.put(name, MethodInfo(MethodVisibility::Public), env);
    return name;
}

// supply an empty array and it will be populated with the method names as symbols
void ModuleObject::methods(Env *env, ArrayObject *array, bool include_super) {
    for (auto pair : m_methods) {
        if (array->include(env, pair.first))
            continue;
        array->push(pair.first);
    }
    if (!include_super) {
        return;
    }
    for (ModuleObject *module : m_included_modules) {
        for (auto pair : module->m_methods) {
            if (array->include(env, pair.first))
                continue;
            array->push(pair.first);
        }
    }
    if (m_superclass) {
        m_superclass->methods(env, array);
    }
}

void ModuleObject::define_method(Env *env, SymbolObject *name, Method *method, MethodVisibility visibility) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    GlobalEnv::the()->increment_method_cache_version();
    m_methods.put(name, MethodInfo(visibility, method), env);
}

// returns the method and sets matching_class_or_module to where the method was found
MethodInfo ModuleObject::find_method(Env *env, SymbolObject *method_name, ModuleObject **matching_class_or_module, const Method **after_method) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    MethodInfo method_info;
    if (!after_method && m_method_cache_version == GlobalEnv::the()->method_cache_version()) {
        method_info = m_method_cache.get(method_name, env);
        if (method_info)
            return method_info;
    }

    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method_info = m_methods.get(method_name, env);
        if (method_info) {
            if (!method_info.is_defined()) {
                if (!after_method)
                    cache_method(method_name, method_info, env);
                return method_info;
            }
            auto method = method_info.method();
            if (after_method != nullptr && method == *after_method) {
                *after_method = nullptr;
            } else if (after_method == nullptr || *after_method == nullptr) {
                if (matching_class_or_module) *matching_class_or_module = m_klass;
                if (!after_method)
                    cache_method(method_name, method_info, env);
                return method_info;
            }
        }
    }

    for (ModuleObject *module : m_included_modules) {
        if (module == this) {
            method_info = module->m_methods.get(method_name, env);
        } else {
            method_info = module->find_method(env, method_name, matching_class_or_module, after_method);
        }
        if (method_info) {
            if (!method_info.is_defined()) {
                if (!after_method)
                    cache_method(method_name, method_info, env);
                return method_info;
            }
            auto method = method_info.method();
            if (after_method != nullptr && method == *after_method) {
                *after_method = nullptr;
            } else if (after_method == nullptr || *after_method == nullptr) {
                if (matching_class_or_module) *matching_class_or_module = module;
                if (!after_method)
                    cache_method(method_name, method_info, env);
                return method_info;
            }
        }
    }

    if (!m_superclass)
        return {};

    method_info = m_superclass->find_method(env, method_name, matching_class_or_module, after_method);
    if (!after_method)
        cache_method(method_name, method_info, env);
    return method_info;
}

MethodInfo ModuleObject::find_method(Env *env, SymbolObject *method_name, const Method *after_method) {
    return find_method(env, method_name, nullptr, &after_method);
}

void ModuleObject::assert_method_defined(Env *env, SymbolObject *name, MethodInfo method_info) {
    if (!method_info.is_defined()) {
        if (type() == Type::Class)
            env->raise_name_error(name, "undefined method `{}' for class `{}'", name->string(), inspect_module());
        else
            env->raise_name_error(name, "undefined method `{}' for module `{}'", name->string(), inspect_module());
    }
}

Value ModuleObject::instance_method(Env *env, Value name_value) {
    auto name = name_value.to_symbol(env, Value::Conversion::Strict);
    auto method_info = find_method(env, name);
    assert_method_defined(env, name, method_info);
    return new UnboundMethodObject { this, method_info.method() };
}

Value ModuleObject::public_instance_method(Env *env, Value name_value) {
    auto name = name_value.to_symbol(env, Value::Conversion::Strict);
    auto method_info = find_method(env, name);
    assert_method_defined(env, name, method_info);

    switch (method_info.visibility()) {
    case MethodVisibility::Public:
        return new UnboundMethodObject { this, method_info.method() };
    case MethodVisibility::Protected:
        if (type() == Type::Class)
            env->raise_name_error(name, "method `{}' for class `{}' is protected", name->string(), inspect_module());
        else
            env->raise_name_error(name, "method `{}' for module `{}' is protected", name->string(), inspect_module());
    case MethodVisibility::Private:
        if (type() == Type::Class)
            env->raise_name_error(name, "method `{}' for class `{}' is private", name->string(), inspect_module());
        else
            env->raise_name_error(name, "method `{}' for module `{}' is private", name->string(), inspect_module());
    default:
        NAT_UNREACHABLE();
    }
}

Value ModuleObject::instance_methods(Env *env, Optional<Value> include_super_value, std::function<bool(MethodVisibility)> predicate) {
    bool include_super = !include_super_value || include_super_value.value().is_truthy();
    ArrayObject *array = new ArrayObject {};
    methods(env, array, include_super);
    array->select_in_place([this, env, predicate](Value &name_value) -> bool {
        auto name = name_value.as_symbol();
        auto method_info = find_method(env, name);
        return method_info.is_defined() && predicate(method_info.visibility());
    });
    return array;
}

Value ModuleObject::instance_methods(Env *env, Optional<Value> include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Public || visibility == MethodVisibility::Protected;
    });
}

Value ModuleObject::private_instance_methods(Env *env, Optional<Value> include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Private;
    });
}

Value ModuleObject::protected_instance_methods(Env *env, Optional<Value> include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Protected;
    });
}

Value ModuleObject::public_instance_methods(Env *env, Optional<Value> include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Public;
    });
}

ArrayObject *ModuleObject::ancestors(Env *env) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ModuleObject *klass = this;
    ArrayObject *ancestors = new ArrayObject {};
    do {
        if (klass->included_modules().is_empty()) {
            // note: if there are included modules, then they will include this klass
            ancestors->push(klass);
        }
        for (ModuleObject *m : klass->included_modules()) {
            ancestors->push(m);
        }
        klass = klass->m_superclass;
    } while (klass);
    return ancestors;
}

bool ModuleObject::ancestors_includes(Env *env, ModuleObject *module) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ModuleObject *klass = this;
    do {
        if (klass->included_modules().is_empty()) {
            // note: if there are included modules, then they will include this klass
            if (klass == module) {
                return true;
            }
        }
        for (ModuleObject *m : klass->included_modules()) {
            if (m == module) {
                return true;
            }
        }
        klass = klass->m_superclass;
    } while (klass);
    return false;
}

bool ModuleObject::is_subclass_of(ModuleObject *other) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (other == this) {
        return false;
    }
    ModuleObject *klass = this;
    do {
        if (other == klass->m_superclass) {
            return true;
        }
        for (ModuleObject *m : klass->included_modules()) {
            if (other == m) {
                return true;
            }
        }
        klass = klass->m_superclass;
    } while (klass);
    return false;
}

bool ModuleObject::is_method_defined(Env *env, Value name_value) {
    auto name = name_value.to_symbol(env, Value::Conversion::Strict);
    return !!find_method(env, name);
}

String ModuleObject::inspect_module() const {
    if (m_name) {
        if (owner() && owner() != GlobalEnv::the()->Object()) {
            return String::format("{}::{}", owner()->inspect_module(), m_name.value());
        } else {
            return String(m_name.value());
        }
    } else if (type() == Type::Class) {
        return String::format("#<Class:{}>", pointer_id());
    } else if (type() == Type::Module && m_name) {
        return String(m_name.value());
    } else {
        return String::format("#<{}:{}>", klass()->inspect_module(), pointer_id());
    }
}

Value ModuleObject::inspect(Env *env) const {
    return new StringObject { inspect_module() };
}

Value ModuleObject::name(Env *env) const {
    if (m_name) {
        String name = m_name.value();
        auto the_owner = owner();
        if (the_owner && the_owner != GlobalEnv::the()->Object()) {
            auto owner_name = the_owner->name();
            if (owner_name) {
                name.prepend("::");
                name.prepend(owner_name.value());
            }
        }
        return new StringObject { name };
    } else {
        return Value::nil();
    }
}

String ModuleObject::backtrace_name() const {
    if (!m_name)
        return inspect_module();
    return String::format("<module:{}>", m_name.value());
}

ArrayObject *ModuleObject::attr(Env *env, Args &&args) {
    bool accessor = false;
    auto size = args.size();
    if (args.size() > 1 && args[size - 1].is_boolean()) {
        env->verbose_warn("optional boolean argument is obsoleted");
        accessor = args[size - 1].is_truthy();
        args.pop(env);
    }
    if (accessor) {
        return attr_accessor(env, Args(args));
    } else {
        return attr_reader(env, Args(args));
    }
}

ArrayObject *ModuleObject::attr_reader(Env *env, Args &&args) {
    auto ary = new ArrayObject { args.size() };
    for (size_t i = 0; i < args.size(); i++) {
        auto name = attr_reader(env, args[i]);
        ary->push(name);
    }
    return ary;
}

SymbolObject *ModuleObject::attr_reader(Env *env, Value obj) {
    auto name = obj.to_symbol(env, Value::Conversion::Strict);
    OwnedPtr<Env> block_env { new Env {} };
    block_env->var_set("name", 0, true, name);
    Block *attr_block = new Block { std::move(block_env), this, ModuleObject::attr_reader_block_fn, 0 };
    define_method(env, name, attr_block);
    return name;
}

Value ModuleObject::attr_reader_block_fn(Env *env, Value self, Args &&args, Block *block) {
    Value name_obj = env->outer()->var_get("name", 0);
    assert(name_obj.is_symbol());
    SymbolObject *ivar_name = SymbolObject::intern(TM::String::format("@{}", name_obj.as_symbol()->string()));
    return Object::ivar_get(env, self, ivar_name);
}

ArrayObject *ModuleObject::attr_writer(Env *env, Args &&args) {
    auto ary = new ArrayObject { args.size() };
    for (size_t i = 0; i < args.size(); i++) {
        auto name = attr_writer(env, args[i]);
        ary->push(name);
    }
    return ary;
}

SymbolObject *ModuleObject::attr_writer(Env *env, Value obj) {
    auto name = obj.to_symbol(env, Value::Conversion::Strict);
    auto method_name = SymbolObject::intern(TM::String::format("{}=", name->string()));
    OwnedPtr<Env> block_env { new Env {} };
    block_env->var_set("name", 0, true, name);
    Block *attr_block = new Block { std::move(block_env), this, ModuleObject::attr_writer_block_fn, 1 };
    define_method(env, method_name, attr_block);
    return method_name;
}

Value ModuleObject::attr_writer_block_fn(Env *env, Value self, Args &&args, Block *block) {
    Value val = args[0];
    Value name_obj = env->outer()->var_get("name", 0);
    assert(name_obj.is_symbol());
    SymbolObject *ivar_name = SymbolObject::intern(TM::String::format("@{}", name_obj.as_symbol()->string()));
    Object::ivar_set(env, self, ivar_name, val);
    return val;
}

ArrayObject *ModuleObject::attr_accessor(Env *env, Args &&args) {
    auto ary = new ArrayObject { args.size() * 2 };
    for (size_t i = 0; i < args.size(); i++) {
        ary->push(attr_reader(env, args[i]));
        ary->push(attr_writer(env, args[i]));
    }
    return ary;
}

void ModuleObject::included_modules(Env *env, ArrayObject *modules) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    for (ModuleObject *m : included_modules()) {
        if (m == this || modules->include(env, m))
            continue;
        modules->push(m);
        m->included_modules(env, modules);
    }
    if (m_superclass) {
        m_superclass->included_modules(env, modules);
    }
}

Value ModuleObject::included_modules(Env *env) {
    ArrayObject *modules = new ArrayObject {};
    included_modules(env, modules);
    return modules;
}

bool ModuleObject::does_include_module(Env *env, Value module) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    module.assert_type(env, Object::Type::Module, "Module");
    for (ModuleObject *m : included_modules()) {
        if (this == m)
            continue;
        if (module == m)
            return true;
        if (m->does_include_module(env, module))
            return true;
    }
    if (m_superclass && m_superclass->does_include_module(env, module))
        return true;
    return false;
}

Value ModuleObject::define_method(Env *env, Value name_value, Optional<Value> method_arg, Block *block) {
    auto name = name_value.to_symbol(env, Value::Conversion::Strict);
    if (method_arg) {
        auto method_value = method_arg.value();
        if (method_value.is_proc()) {
            define_method(env, name, method_value.as_proc()->block());
        } else {
            Method *method;
            if (method_value.is_method()) {
                method = method_value.as_method()->method();
            } else if (method_value.is_unbound_method()) {
                method = method_value.as_unbound_method()->method();
            } else {
                env->raise("TypeError", "wrong argument type {} (expected Proc/Method/UnboundMethod)", method_value.klass()->inspect_module());
            }
            ModuleObject *owner = method->owner();
            if (owner != this && owner->type() == Type::Class && !owner->is_subclass_of(this)) {
                if (owner->as_class()->is_singleton())
                    env->raise("TypeError", "can't bind singleton method to a different class");
                else
                    env->raise("TypeError", "bind argument must be a subclass of {}", owner->inspect_module());
            }
            define_method(env, name, method->fn(), method->arity());
        }
    } else if (block) {
        define_method(env, name, block);
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
    return name;
}

Value ModuleObject::module_eval(Env *env, Block *block) {
    if (!block) {
        env->raise("ArgumentError", "Natalie only supports module_eval with a block");
    }
    Value self = this;
    block->set_self(self);
    auto old_method_visibility = m_method_visibility;
    auto old_module_function = m_module_function;
    Value args[] = { self };
    Value result = block->run(env, Args(1, args), nullptr);
    m_method_visibility = old_method_visibility;
    m_module_function = old_module_function;
    return result;
}

Value ModuleObject::module_exec(Env *env, Args &&args, Block *block) {
    if (!block)
        env->raise_local_jump_error(Value::nil(), Natalie::LocalJumpErrorType::None);
    Value self = this;
    block->set_self(self);
    return block->run(env, std::move(args), nullptr);
}

Value ModuleObject::private_method(Env *env, Args &&args) {
    set_method_visibility(env, std::move(args), MethodVisibility::Private);
    return this;
}

Value ModuleObject::protected_method(Env *env, Args &&args) {
    set_method_visibility(env, std::move(args), MethodVisibility::Protected);
    return this;
}

Value ModuleObject::public_method(Env *env, Args &&args) {
    set_method_visibility(env, std::move(args), MethodVisibility::Public);
    return this;
}

Value ModuleObject::private_class_method(Env *env, Args &&args) {
    singleton_class(env, this)->set_method_visibility(env, std::move(args), MethodVisibility::Private);
    return this;
}

Value ModuleObject::public_class_method(Env *env, Args &&args) {
    singleton_class(env, this)->set_method_visibility(env, std::move(args), MethodVisibility::Public);
    return this;
}

void ModuleObject::set_method_visibility(Env *env, Args &&args, MethodVisibility visibility) {
    // private (no args)
    if (args.size() == 0) {
        m_method_visibility = visibility;
        m_module_function = false;
    }

    // private [:foo, :bar]
    if (args.size() == 1 && args[0].is_array()) {
        auto array = args[0].as_array();
        for (auto &value : *array) {
            auto name = value.to_symbol(env, Value::Conversion::Strict);
            set_method_visibility(env, name, visibility);
        }
        return;
    }

    // private :foo, :bar
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        set_method_visibility(env, name, visibility);
    }
}

void ModuleObject::set_method_visibility(Env *env, SymbolObject *name, MethodVisibility visibility) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ModuleObject *matching_class_or_module = nullptr;
    auto method_info = find_method(env, name, &matching_class_or_module);
    assert_method_defined(env, name, method_info);
    GlobalEnv::the()->increment_method_cache_version();
    m_methods.put(name, MethodInfo(visibility, method_info.method()));
}

Value ModuleObject::module_function(Env *env, Args &&args) {
    if (type() == Type::Class)
        env->raise("TypeError", "module_function must be called for modules");

    if (args.size() > 0) {
        for (size_t i = 0; i < args.size(); ++i) {
            std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
            auto name = args[i].to_symbol(env, Value::Conversion::Strict);
            auto method_info = find_method(env, name);
            assert_method_defined(env, name, method_info);
            auto method = method_info.method();
            Object::define_singleton_method(env, this, name, method->fn(), method->arity());
            GlobalEnv::the()->increment_method_cache_version();
            m_methods.put(name, MethodInfo(MethodVisibility::Private, method));
        }
    } else {
        m_method_visibility = MethodVisibility::Private;
        m_module_function = true;
    }
    return this;
}

Value ModuleObject::deprecate_constant(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_module(), name->string());
        constant->set_deprecated(true);
    }
    return this;
}

Value ModuleObject::private_constant(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_module(), name->string());
        constant->set_private(true);
    }
    return this;
}

Value ModuleObject::public_constant(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_module(), name->string());
        constant->set_private(false);
    }
    return this;
}

bool ModuleObject::const_defined(Env *env, Value name_value, Optional<Value> inherited) {
    auto name = name_value.to_symbol(env, Value::Conversion::NullAllowed);
    if (!name) {
        env->raise("TypeError", "no implicit conversion of {} to String", name_value.inspected(env));
    }
    if (inherited && inherited.value().is_falsey()) {
        return !!m_constants.get(name);
    }
    return const_find(env, name, ConstLookupSearchMode::NotStrict, ConstLookupFailureMode::None).present();
}

Value ModuleObject::alias_method(Env *env, Value new_name_value, Value old_name_value) {
    auto new_name = new_name_value.to_symbol(env, Value::Conversion::NullAllowed);
    if (!new_name) {
        env->raise("TypeError", "{} is not a symbol", new_name_value.inspected(env));
    }
    auto old_name = old_name_value.to_symbol(env, Value::Conversion::NullAllowed);
    if (!old_name) {
        env->raise("TypeError", "{} is not a symbol", old_name_value.inspected(env));
    }
    make_method_alias(env, new_name, old_name);
    return new_name;
}

Value ModuleObject::remove_method(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        auto method = m_methods.get(name, env);
        if (!method)
            env->raise_name_error(name, "method `{}' not defined in {}", name->string(), this->inspect_module());
        GlobalEnv::the()->increment_method_cache_version();
        m_methods.remove(name, env);
    }
    return this;
}

Value ModuleObject::undef_method(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto name = args[i].to_symbol(env, Value::Conversion::Strict);
        auto method_info = find_method(env, name);
        assert_method_defined(env, name, method_info);
        undefine_method(env, name);
    }
    return this;
}

Value ModuleObject::ruby2_keywords(Env *env, Value name) {
    if (name.is_string()) {
        name = name.as_string()->to_sym(env);
    } else if (!name.is_symbol()) {
        env->raise("TypeError", "{} is not a symbol nor a string", name.inspected(env));
    }

    auto method_wrapper = [](Env *env, Value self, Args &&args, Block *block) -> Value {
        auto kwargs = args.has_keyword_hash() ? args.pop_keyword_hash() : new HashObject;
        auto new_args = args.to_array_for_block(env, 0, -1, true);
        if (!kwargs->is_empty())
            new_args->push(HashObject::ruby2_keywords_hash(env, kwargs));
        auto old_method = env->outer()->var_get("old_method", 1).as_unbound_method();
        return old_method->bind_call(env, self, std::move(new_args), block);
    };

    OwnedPtr<Env> inner_env { new Env { *env } };
    inner_env->var_set("old_method", 1, true, instance_method(env, name));
    undef_method(env, { name });
    define_method(env, name.as_symbol(), new Block { std::move(inner_env), this, method_wrapper, -1 });

    return Value::nil();
}

void ModuleObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    visitor.visit(m_superclass);
    visitor.visit(m_owner);
    for (auto pair : m_constants) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto pair : m_methods) {
        visitor.visit(pair.first);
        pair.second.visit_children(visitor);
    }
    for (auto pair : m_method_cache) {
        visitor.visit(pair.first);
        pair.second.visit_children(visitor);
    }
    for (auto pair : m_class_vars) {
        visitor.visit(pair.first);
        if (pair.second)
            visitor.visit(pair.second.value());
    }
    for (auto module : m_included_modules) {
        visitor.visit(module);
    }
}

void ModuleObject::cache_method(SymbolObject *method_name, MethodInfo method_info, Env *env) {
    if (m_method_cache_version != GlobalEnv::the()->method_cache_version()) {
        m_method_cache.clear();
        m_method_cache_version = GlobalEnv::the()->method_cache_version();
    }

    m_method_cache.put(method_name, method_info, env);
}

}
