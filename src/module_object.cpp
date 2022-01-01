#include "natalie.hpp"

namespace Natalie {

ModuleObject::ModuleObject()
    : ModuleObject { Object::Type::Module, GlobalEnv::the()->Module() } { }

ModuleObject::ModuleObject(const char *name)
    : ModuleObject {} {
    this->set_class_name(name);
}

ModuleObject::ModuleObject(Type type, ClassObject *klass)
    : Object { type, klass }
    , m_env { new Env() } { }

Value ModuleObject::initialize(Env *env, Block *block) {
    if (block) {
        Value self = this;
        block->set_self(self);
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    }
    return this;
}

Value ModuleObject::include(Env *env, size_t argc, Value *args) {
    for (int i = argc - 1; i >= 0; i--) {
        include_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleObject::include_once(Env *env, ModuleObject *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
        m_included_modules.push(module);
    } else {
        int index = 0;
        while (m_included_modules[index] != this) {
            index += 1;
        }
        m_included_modules.insert(index + 1, module);
    }
}

Value ModuleObject::prepend(Env *env, size_t argc, Value *args) {
    for (int i = argc - 1; i >= 0; i--) {
        prepend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleObject::prepend_once(Env *env, ModuleObject *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(module);
        m_included_modules.push(this);
    } else {
        m_included_modules.push_front(module);
    }
}

Value ModuleObject::const_get(SymbolObject *name) {
    auto constant = m_constants.get(name);
    if (constant)
        return constant->value();
    else
        return nullptr;
}

Value ModuleObject::const_fetch(SymbolObject *name) {
    auto constant = const_get(name);
    if (!constant) {
        printf("Constant %s is missing!\n", name->c_str());
        abort();
    }
    return constant;
}

Value ModuleObject::const_find(Env *env, SymbolObject *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    ModuleObject *search_parent;
    Constant *constant;

    if (search_mode == ConstLookupSearchMode::NotStrict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = this;
        search_parent->m_constants.get(name);
        while (!(constant = search_parent->m_constants.get(name)) && search_parent->owner() && search_parent->owner() != GlobalEnv::the()->Object()) {
            search_parent = search_parent->owner();
        }
        if (constant) {
            if (constant->is_deprecated()) {
                env->warn("constant ::{} is deprecated", name->c_str());
            }
            return constant->value();
        }
    }

    // search in superclass hierarchy
    search_parent = this;
    do {
        constant = search_parent->m_constants.get(name);
        if (constant) break;
        search_parent = search_parent->m_superclass;
    } while (search_parent && search_parent != GlobalEnv::the()->Object());

    if (constant) {
        if (search_mode == ConstLookupSearchMode::Strict && constant->is_private())
            env->raise_name_error(name, "private constant {}::{} referenced", search_parent->inspect_str(), name->c_str());
        if (constant->is_deprecated()) {
            env->warn("constant {}::{} is deprecated", search_parent->inspect_str(), name->c_str());
        }
        return constant->value();
    }

    if (this != GlobalEnv::the()->Object() && search_mode == ConstLookupSearchMode::NotStrict) {
        // lastly, search on the global, i.e. Object namespace
        constant = GlobalEnv::the()->Object()->m_constants.get(name);
        if (constant) return constant->value();
    }

    if (failure_mode == ConstLookupFailureMode::Null) return nullptr;

    if (search_mode == ConstLookupSearchMode::Strict) {
        env->raise_name_error(name, "uninitialized constant {}::{}", this->inspect_str(), name->c_str());
    } else {
        env->raise_name_error(name, "uninitialized constant {}", name->c_str());
    }
}

Value ModuleObject::const_set(SymbolObject *name, Value val) {
    m_constants.put(name, new Constant { name, val.object() });
    if (val->is_module() && !val->owner()) {
        val->set_owner(this);
        if (val->singleton_class()) val->singleton_class()->set_owner(this);
    }
    return val;
}

Value ModuleObject::const_set(Env *env, Value name, Value val) {
    return const_set(name->to_symbol(env, Object::Conversion::Strict), val);
}

void ModuleObject::alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    Method *method = find_method(env, old_name);
    assert_method_defined(env, old_name, method);
    define_method(env, new_name, method, get_method_visibility(env, old_name));
}

Value ModuleObject::eval_body(Env *env, Value (*fn)(Env *, Value)) {
    Env body_env { m_env };
    body_env.set_caller(env);
    Value result = fn(&body_env, this);
    m_method_visibility = MethodVisibility::Public;
    m_module_function = false;
    return result;
}

Value ModuleObject::cvar_get_or_null(Env *env, SymbolObject *name) {
    if (!name->is_cvar_name())
        env->raise_name_error(name, "`{}' is not allowed as a class variable name", name->c_str());

    ModuleObject *module = this;
    Value val = nullptr;
    while (module) {
        val = module->m_class_vars.get(name, env);
        if (val)
            return val;
        module = module->m_superclass;
    }
    return nullptr;
}

Value ModuleObject::cvar_set(Env *env, SymbolObject *name, Value val) {
    if (!name->is_cvar_name())
        env->raise_name_error(name, "`{}' is not allowed as a class variable name", name->c_str());

    ModuleObject *current = this;

    Value exists = nullptr;
    while (current) {
        exists = current->m_class_vars.get(name, env);
        if (exists) {
            current->m_class_vars.put(name, val.object(), env);
            return val;
        }
        current = current->m_superclass;
    }
    m_class_vars.put(name, val.object(), env);
    return val;
}

SymbolObject *ModuleObject::define_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity) {
    Method *method = new Method { name->c_str(), this, fn, arity };
    define_method(env, name, method, m_method_visibility);
    if (m_module_function)
        define_singleton_method(env, name, fn, arity);
    return name;
}

SymbolObject *ModuleObject::define_method(Env *env, SymbolObject *name, Block *block) {
    Method *method = new Method { name->c_str(), this, block };
    define_method(env, name, method, m_method_visibility);
    if (m_module_function)
        define_singleton_method(env, name, block);
    return name;
}

SymbolObject *ModuleObject::undefine_method(Env *env, SymbolObject *name) {
    return define_method(env, name, nullptr, 0);
}

// supply an empty array and it will be populated with the method names as symbols
void ModuleObject::methods(Env *env, ArrayObject *array, bool include_super) {
    for (auto pair : m_methods) {
        array->push(pair.first);
    }
    if (!include_super) {
        return;
    }
    for (ModuleObject *module : m_included_modules) {
        for (auto pair : module->m_methods) {
            array->push(pair.first);
        }
    }
    if (m_superclass) {
        m_superclass->methods(env, array);
    }
}

void ModuleObject::define_method(Env *env, SymbolObject *name, Method *method, MethodVisibility visibility) {
    m_methods.put(name, method, env);
    set_method_visibility(env, name, visibility);
}

void ModuleObject::set_method_visibility(Env *env, SymbolObject *name, MethodVisibility visibility) {
    auto info = m_method_info.get(name, env);
    if (info) {
        info->set_visibility(visibility);
    } else {
        info = new MethodInfo { name->c_str(), visibility };
        m_method_info.put(name, info, env);
    }
}

MethodVisibility ModuleObject::get_method_visibility(Env *env, SymbolObject *name) {
    auto info = find_method_info(env, name);
    if (!info) {
        env->raise_name_error(name, "undefined method `{}' for `{}'", name->c_str(), inspect_str());
    }
    return info->visibility();
}

MethodInfo *ModuleObject::find_method_info(Env *env, SymbolObject *method_name) {
    MethodInfo *info;
    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        info = m_method_info.get(method_name, env);
        if (info)
            return info;
    }

    for (ModuleObject *module : m_included_modules) {
        if (module == this)
            info = module->m_method_info.get(method_name, env);
        else
            info = module->find_method_info(env, method_name);
        if (info)
            return info;
    }

    if (m_superclass) {
        return m_superclass->find_method_info(env, method_name);
    } else {
        return nullptr;
    }
}

// returns the method and sets matching_class_or_module to where the method was found
Method *ModuleObject::find_method(Env *env, SymbolObject *method_name, ModuleObject **matching_class_or_module, Method **after_method) const {
    Method *method;
    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = m_methods.get(method_name, env);
        if (method) {
            if (after_method != nullptr && method == *after_method) {
                *after_method = nullptr;
            } else if (after_method == nullptr || *after_method == nullptr) {
                if (matching_class_or_module) *matching_class_or_module = m_klass;
                return method;
            }
        }
    }

    for (ModuleObject *module : m_included_modules) {
        if (module == this)
            method = module->m_methods.get(method_name, env);
        else
            method = module->find_method(env, method_name, matching_class_or_module, after_method);
        if (method) {
            if (after_method != nullptr && method == *after_method) {
                *after_method = nullptr;
            } else if (after_method == nullptr || *after_method == nullptr) {
                if (matching_class_or_module) *matching_class_or_module = module;
                return method;
            }
        }
    }

    if (m_superclass) {
        return m_superclass->find_method(env, method_name, matching_class_or_module, after_method);
    } else {
        return nullptr;
    }
}

Method *ModuleObject::find_method(Env *env, SymbolObject *method_name, Method *after_method) const {
    return find_method(env, method_name, nullptr, &after_method);
}

void ModuleObject::assert_method_defined(Env *env, SymbolObject *name, Method *method) {
    if (!method || method->is_undefined()) {
        if (is_class()) {
            env->raise_name_error(name, "undefined method `{}' for class `{}'", name->c_str(), inspect_str());
        } else {
            env->raise_name_error(name, "undefined method `{}' for module `{}'", name->c_str(), inspect_str());
        }
    }
}

Value ModuleObject::instance_method(Env *env, Value name_value) {
    auto name = name_value->to_symbol(env, Object::Conversion::Strict);
    auto method = find_method(env, name);
    assert_method_defined(env, name, method);
    return new UnboundMethodObject { this, method };
}

Value ModuleObject::public_instance_method(Env *env, Value name_value) {
    auto name = name_value->to_symbol(env, Object::Conversion::Strict);
    auto method = find_method(env, name);
    assert_method_defined(env, name, method);

    auto visibility = get_method_visibility(env, name);
    switch (visibility) {
    case MethodVisibility::Public:
        return new UnboundMethodObject { this, method };
    case MethodVisibility::Protected:
        if (is_class()) {
            env->raise_name_error(name, "method `{}' for class `{}' is protected", name->c_str(), inspect_str());
        } else {
            env->raise_name_error(name, "method `{}' for module `{}' is protected", name->c_str(), inspect_str());
        }
    case MethodVisibility::Private:
        if (is_class()) {
            env->raise_name_error(name, "method `{}' for class `{}' is private", name->c_str(), inspect_str());
        } else {
            env->raise_name_error(name, "method `{}' for module `{}' is private", name->c_str(), inspect_str());
        }
    default:
        NAT_UNREACHABLE();
    }
}

Value ModuleObject::instance_methods(Env *env, Value include_super_value, std::function<bool(MethodVisibility)> predicate) {
    bool include_super = !include_super_value || include_super_value->is_truthy();
    ArrayObject *array = new ArrayObject {};
    methods(env, array, include_super);
    array->uniq_in_place(env, nullptr);
    array->select_in_place([this, env, predicate](Value &name_value) -> bool {
        auto name = name_value->as_symbol();
        auto method = find_method(env, name);
        return method && !method->is_undefined() && predicate(get_method_visibility(env, name));
    });
    return array;
}

Value ModuleObject::instance_methods(Env *env, Value include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Public || visibility == MethodVisibility::Protected;
    });
}

Value ModuleObject::private_instance_methods(Env *env, Value include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Private;
    });
}

Value ModuleObject::protected_instance_methods(Env *env, Value include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Protected;
    });
}

Value ModuleObject::public_instance_methods(Env *env, Value include_super_value) {
    return instance_methods(env, include_super_value, [](MethodVisibility visibility) {
        return visibility == MethodVisibility::Public;
    });
}

ArrayObject *ModuleObject::ancestors(Env *env) {
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

bool ModuleObject::is_subclass_of(ModuleObject *other) {
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

bool ModuleObject::is_method_defined(Env *env, Value name_value) const {
    auto name = name_value->to_symbol(env, Conversion::Strict);
    return !!find_method(env, name);
}

const String *ModuleObject::inspect_str() {
    if (m_class_name) {
        if (owner() && owner() != GlobalEnv::the()->Object()) {
            return String::format("{}::{}", owner()->inspect_str(), m_class_name.value());
        } else {
            return m_class_name.value();
        }
    } else if (is_class()) {
        return String::format("#<Class:{}>", pointer_id());
    } else if (is_module() && m_class_name) {
        return m_class_name.value();
    } else {
        return String::format("#<{}:{}>", klass()->inspect_str(), pointer_id());
    }
}

Value ModuleObject::inspect(Env *env) {
    return new StringObject { *inspect_str() };
}

Value ModuleObject::name(Env *env) {
    if (m_class_name) {
        return new StringObject { *m_class_name.value() };
    } else {
        return NilObject::the();
    }
}

Value ModuleObject::attr_reader(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; i++) {
        Value name_obj = args[i];
        if (name_obj->type() == Object::Type::String) {
            // we're good!
        } else if (name_obj->type() == Object::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "{} is not a symbol nor a string", name_obj->inspect_str(env));
        }
        auto block_env = new Env {};
        block_env->var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleObject::attr_reader_block_fn, 0 };
        define_method(env, name_obj->as_string()->to_symbol(env), attr_block);
    }
    return NilObject::the();
}

Value ModuleObject::attr_reader_block_fn(Env *env, Value self, size_t argc, Value *args, Block *block) {
    Value name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    assert(name_obj->is_string());
    StringObject *ivar_name = StringObject::format(env, "@{}", name_obj->as_string());
    return self->ivar_get(env, ivar_name->to_symbol(env));
}

Value ModuleObject::attr_writer(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; i++) {
        Value name_obj = args[i];
        if (name_obj->type() == Object::Type::String) {
            // we're good!
        } else if (name_obj->type() == Object::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "{} is not a symbol nor a string", name_obj->inspect_str(env));
        }
        StringObject *method_name = new StringObject { name_obj->as_string()->c_str() };
        method_name->append_char('=');
        auto block_env = new Env {};
        block_env->var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleObject::attr_writer_block_fn, 0 };
        define_method(env, method_name->to_symbol(env), attr_block);
    }
    return NilObject::the();
}

Value ModuleObject::attr_writer_block_fn(Env *env, Value self, size_t argc, Value *args, Block *block) {
    Value val = args[0];
    Value name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    assert(name_obj->is_string());
    StringObject *ivar_name = StringObject::format(env, "@{}", name_obj->as_string());
    self->ivar_set(env, ivar_name->to_symbol(env), val);
    return val;
}

Value ModuleObject::attr_accessor(Env *env, size_t argc, Value *args) {
    attr_reader(env, argc, args);
    attr_writer(env, argc, args);
    return NilObject::the();
}

Value ModuleObject::included_modules(Env *env) {
    ArrayObject *modules = new ArrayObject { included_modules().size() };
    for (ModuleObject *m : included_modules()) {
        modules->push(m);
    }
    return modules;
}

bool ModuleObject::does_include_module(Env *env, Value module) {
    module->assert_type(env, Object::Type::Module, "Module");
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

Value ModuleObject::define_method(Env *env, Value name_value, Value method_value, Block *block) {
    auto name = name_value->to_symbol(env, Object::Conversion::Strict);
    if (method_value) {
        if (method_value->is_proc()) {
            define_method(env, name, method_value->as_proc()->block());
        } else {
            Method *method;
            if (method_value->is_method()) {
                method = method_value->as_method()->method();
            } else if (method_value->is_unbound_method()) {
                method = method_value->as_unbound_method()->method();
            } else {
                env->raise("TypeError", "wrong argument type {} (expected Proc/Method/UnboundMethod)", method_value->klass()->inspect_str());
            }
            ModuleObject *owner = method->owner();
            if (owner != this && owner->is_class() && !owner->is_subclass_of(this)) {
                if (owner->as_class()->is_singleton()) {
                    env->raise("TypeError", "can't bind singleton method to a different class");
                } else {
                    env->raise("TypeError", "bind argument must be a subclass of {}", owner->inspect_str());
                }
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
    Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    m_method_visibility = old_method_visibility;
    m_module_function = old_module_function;
    return result;
}

Value ModuleObject::private_method(Env *env, size_t argc, Value *args) {
    set_method_visibility(env, argc, args, MethodVisibility::Private);
    return this;
}

Value ModuleObject::protected_method(Env *env, size_t argc, Value *args) {
    set_method_visibility(env, argc, args, MethodVisibility::Protected);
    return this;
}

Value ModuleObject::public_method(Env *env, size_t argc, Value *args) {
    set_method_visibility(env, argc, args, MethodVisibility::Public);
    return this;
}

Value ModuleObject::private_class_method(Env *env, size_t argc, Value *args) {
    singleton_class(env)->set_method_visibility(env, argc, args, MethodVisibility::Private);
    return this;
}

Value ModuleObject::public_class_method(Env *env, size_t argc, Value *args) {
    singleton_class(env)->set_method_visibility(env, argc, args, MethodVisibility::Public);
    return this;
}

void ModuleObject::set_method_visibility(Env *env, size_t argc, Value *args, MethodVisibility visibility) {
    if (argc > 0) {
        if (argc == 1 && args[0]->is_array()) {
            auto array = args[0]->as_array();
            argc = array->size();
            args = array->data();
        }
        for (size_t i = 0; i < argc; ++i) {
            auto name = args[i]->to_symbol(env, Conversion::Strict);
            auto method = find_method(env, name);
            assert_method_defined(env, name, method);
            set_method_visibility(env, name, visibility);
        }
    } else {
        m_method_visibility = visibility;
        m_module_function = false;
    }
}

Value ModuleObject::module_function(Env *env, size_t argc, Value *args) {
    if (is_class()) {
        env->raise("TypeError", "module_function must be called for modules");
    }
    if (argc > 0) {
        for (size_t i = 0; i < argc; ++i) {
            auto name = args[i]->to_symbol(env, Conversion::Strict);
            auto method = find_method(env, name);
            assert_method_defined(env, name, method);
            define_singleton_method(env, name, method->fn(), method->arity());
            set_method_visibility(env, name, MethodVisibility::Private);
        }
    } else {
        m_method_visibility = MethodVisibility::Private;
        m_module_function = true;
    }
    return this;
}

Value ModuleObject::deprecate_constant(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; ++i) {
        auto name = args[i]->to_symbol(env, Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_str(), name->c_str());
        constant->set_deprecated(true);
    }
    return this;
}

Value ModuleObject::private_constant(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; ++i) {
        auto name = args[i]->to_symbol(env, Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_str(), name->c_str());
        constant->set_private(true);
    }
    return this;
}

Value ModuleObject::public_constant(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; ++i) {
        auto name = args[i]->to_symbol(env, Conversion::Strict);
        auto constant = m_constants.get(name);
        if (!constant)
            env->raise_name_error(name, "constant {}::{} not defined", inspect_str(), name->c_str());
        constant->set_private(false);
    }
    return this;
}

bool ModuleObject::const_defined(Env *env, Value name_value) {
    auto name = name_value->to_symbol(env, Object::Conversion::NullAllowed);
    if (!name) {
        env->raise("TypeError", "no implicit conversion of {} to String", name_value->inspect_str(env));
    }
    return !!const_find(env, name, ConstLookupSearchMode::NotStrict, ConstLookupFailureMode::Null);
}

Value ModuleObject::alias_method(Env *env, Value new_name_value, Value old_name_value) {
    auto new_name = new_name_value->to_symbol(env, Object::Conversion::NullAllowed);
    if (!new_name) {
        env->raise("TypeError", "{} is not a symbol", new_name_value->inspect_str(env));
    }
    auto old_name = old_name_value->to_symbol(env, Object::Conversion::NullAllowed);
    if (!old_name) {
        env->raise("TypeError", "{} is not a symbol", old_name_value->inspect_str(env));
    }
    alias(env, new_name, old_name);
    return this;
}

Value ModuleObject::remove_method(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; ++i) {
        auto name = args[i]->to_symbol(env, Conversion::Strict);
        auto info = m_method_info.get(name, env);
        if (!info) {
            env->raise_name_error(name, "method `{}' not defined in {}", name->c_str(), this->inspect_str());
        }
        m_methods.remove(name, env);
        m_method_info.remove(name, env);
    }
    return this;
}

Value ModuleObject::undef_method(Env *env, size_t argc, Value *args) {
    for (size_t i = 0; i < argc; ++i) {
        auto name = args[i]->to_symbol(env, Conversion::Strict);
        auto method = find_method(env, name);
        assert_method_defined(env, name, method);
        undefine_method(env, name);
    }
    return this;
}

void ModuleObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_env);
    visitor.visit(m_superclass);
    if (m_class_name)
        visitor.visit(m_class_name.value());
    for (auto pair : m_constants) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto pair : m_methods) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto pair : m_method_info) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto pair : m_class_vars) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto module : m_included_modules) {
        visitor.visit(module);
    }
}

}
