#include <dlfcn.h>
#include <ffi.h>

#include "natalie.hpp"

using namespace Natalie;

Value init(Env *env, Value self) {
    return NilObject::the();
}

Value FFI_Library_ffi_lib(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto name = args.at(0);
    name->assert_type(env, Object::Type::String, "String");
    auto handle = dlopen(name->as_string()->c_str(), RTLD_LAZY);
    if (!handle) {
        char buf[1000];
        snprintf(buf, 1000, "Could not open library '%s': %s.", name->as_string()->c_str(), dlerror());
        env->raise("LoadError", buf);
    }
    auto handle_ptr = new VoidPObject { handle, [](auto p) { dlclose(p->void_ptr()); } };
    auto libs = self->ivar_get(env, "@ffi_libs"_s);
    if (libs->is_nil())
        libs = self->ivar_set(env, "@ffi_libs"_s, new ArrayObject);
    auto DynamicLibrary = fetch_nested_const({ "FFI"_s, "DynamicLibrary"_s });
    auto lib = DynamicLibrary->send(env, "new"_s, { name, handle_ptr });
    libs->as_array()->push(lib);
    return NilObject::the();
}

static ffi_type *get_ffi_type(Env *env, Value type) {
    type->assert_type(env, Object::Type::Symbol, "Symbol");
    auto type_sym = type->as_symbol();
    if (type_sym == "bool"_s) {
        return &ffi_type_ushort;
    } else if (type_sym == "char"_s) {
        return &ffi_type_uchar;
    } else if (type_sym == "pointer"_s) {
        return &ffi_type_pointer;
    } else if (type_sym == "size_t"_s) {
        return &ffi_type_uint64;
    } else if (type_sym == "void"_s) {
        return &ffi_type_void;
    } else {
        env->raise("TypeError", "unable to resolve type '{}'", type_sym->string());
    }
}

typedef union {
    void *vp;
    unsigned short us;
    unsigned char uc;
} FFI_Library_call_arg_slot;

static Value FFI_Library_fn_call_block(Env *env, Value self, Args args, Block *block) {
    Value cif_obj = env->outer()->var_get("cif", 0);
    auto cif = (ffi_cif *)cif_obj->as_void_p()->void_ptr();
    assert(cif);

    auto arg_types = env->outer()->var_get("arg_types", 1)->as_array();
    auto return_type = env->outer()->var_get("arg_types", 2)->as_symbol();

    Value fn_obj = env->outer()->var_get("fn", 4);
    auto fn = (void (*)())fn_obj->as_void_p()->void_ptr();
    assert(fn);

    args.ensure_argc_is(env, cif->nargs);

    auto MemoryPointer = fetch_nested_const({ "FFI"_s, "MemoryPointer"_s })->as_class();

    auto bool_sym = "bool"_s;
    auto char_sym = "char"_s;
    auto pointer_sym = "pointer"_s;
    auto size_t_sym = "size_t"_s;
    auto void_sym = "void"_s;

    FFI_Library_call_arg_slot arg_values[cif->nargs];
    void *arg_pointers[cif->nargs];

    for (unsigned i = 0; i < cif->nargs; ++i) {
        auto val = args.at(i);
        auto type = arg_types->at(i);
        if (type == pointer_sym) {
            if (val->klass() == MemoryPointer) {
                arg_values[i].vp = val->ivar_get(env, "@ptr"_s)->as_void_p()->void_ptr();
                arg_pointers[i] = &(arg_values[i].vp);
            } else {
                env->raise("ArgumentError", "Expected MemoryPointer but got {}", val);
            }
        } else if (type == bool_sym) {
            arg_values[i].us = val->is_truthy() ? 1 : 0;
            arg_pointers[i] = &(arg_values[i].us);
        } else if (type == char_sym) {
            auto string = val->as_string_or_raise(env);
            if (!string->is_empty())
                arg_values[i].uc = string->at(0);
            else
                arg_values[i].uc = 0;
            arg_pointers[i] = &(arg_values[i].uc);
        } else {
            env->raise("StandardError", "I don't yet know how to handle argument type {}", val->klass()->name());
        }
    }

    ffi_arg result;
    ffi_call(cif, fn, &result, arg_pointers);

    if (return_type == bool_sym) {
        return bool_object((uint64_t)result);
    } else if (return_type == char_sym) {
        TM::String c = (char)result;
        return new StringObject { std::move(c) };
    } else if (return_type == pointer_sym) {
        auto Pointer = fetch_nested_const({ "FFI"_s, "Pointer"_s })->as_class();
        auto pointer = Pointer->send(env, "new"_s);
        pointer->ivar_set(env, "@address"_s, new VoidPObject { (void *)result });
        return pointer;
    } else if (return_type == size_t_sym) {
        assert((uint64_t)result <= std::numeric_limits<nat_int_t>::max());
        return Value::integer((nat_int_t)result);
    } else if (return_type == void_sym) {
        return NilObject::the();
    } else {
        env->raise("StandardError", "I don't yet know how to handle return type {}", return_type->string());
    }
}

Value FFI_Library_attach_function(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 3);
    auto name = args.at(0)->to_symbol(env, Object::Conversion::Strict);
    auto arg_types = args.at(1);
    arg_types->assert_type(env, Object::Type::Array, "Array");
    auto return_type = args.at(2)->to_symbol(env, Object::Conversion::Strict);
    // dbg("attach_function {v} {v} {v}", name, arg_types, return_type);

    auto arg_types_array = arg_types->as_array();
    auto arg_count = arg_types_array->size();

    auto ffi_args = new ffi_type *[arg_count];
    for (size_t i = 0; i < arg_count; ++i) {
        ffi_args[i] = get_ffi_type(env, arg_types_array->at(i));
    }
    auto ffi_args_obj = new VoidPObject {
        ffi_args,
        [](auto p) {
            auto ary = (ffi_type **)p->void_ptr();
            delete[] ary;
        }
    };

    auto libs = self->ivar_get(env, "@ffi_libs"_s);
    auto lib = libs->as_array()->first(); // what do we do if there is more than one?
    auto handle = lib->ivar_get(env, "@lib"_s)->as_void_p()->void_ptr();

    dlerror(); // clear any previous error
    auto fn = dlsym(handle, name->string().c_str());
    auto error = dlerror();
    if (error) {
        auto NotFoundError = fetch_nested_const({ "FFI"_s, "NotFoundError"_s })->as_class();
        auto message = StringObject::format("Function '{}' not found in [{}]", name->string(), lib->send(env, "name"_s)->as_string());
        auto exception = NotFoundError->send(env, "new"_s, { message })->as_exception();
        env->raise_exception(exception);
    }

    auto cif = new ffi_cif;
    auto status = ffi_prep_cif(
        cif,
        FFI_DEFAULT_ABI,
        arg_count,
        get_ffi_type(env, return_type),
        ffi_args);

    if (status != FFI_OK)
        env->raise("LoadError", "There was an error preparing the FFI call data structure: {}", (int)status);

    auto block_env = new Env {};
    block_env->var_set("cif", 0, true, new VoidPObject { cif });
    block_env->var_set("arg_types", 1, true, arg_types_array);
    block_env->var_set("return_type", 2, true, return_type);
    block_env->var_set("ffi_args", 3, true, ffi_args_obj);
    block_env->var_set("fn", 4, true, new VoidPObject { fn });
    Block *block = new Block { block_env, self, FFI_Library_fn_call_block, 0 };
    self->define_singleton_method(env, name, block);

    return NilObject::the();
}

Value FFI_Pointer_address(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    auto address = self->ivar_get(env, "@address"_s)->as_void_p()->void_ptr();
    return Value::integer((nat_int_t)address);
}

Value FFI_Pointer_read_string(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    auto address = self->ivar_get(env, "@address"_s)->as_void_p()->void_ptr();
    return new StringObject { (char *)address };
}

Value FFI_MemoryPointer_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);

    auto size = args.at(0);
    size->assert_type(env, Object::Type::Integer, "Integer");
    self->ivar_set(env, "@size"_s, size);

    auto ptr = malloc(size->as_integer()->to_nat_int_t());
    auto ptr_obj = new VoidPObject {
        ptr,
        [](auto p) {
            free(p->void_ptr());
        }
    };
    self->ivar_set(env, "@ptr"_s, ptr_obj);

    return NilObject::the();
}

Value FFI_MemoryPointer_free(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    auto ptr = self->ivar_get(env, "@ptr"_s)->as_void_p();
    free(ptr->void_ptr());
    return NilObject::the();
}

Value FFI_MemoryPointer_inspect(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    auto ptr = self->ivar_get(env, "@ptr"_s)->as_void_p();
    auto size = self->ivar_get(env, "@size"_s)->as_integer();
    return StringObject::format(
        "#<FFI::MemoryPointer address={} size={}>",
        TM::String::hex((uintptr_t)ptr->void_ptr(), TM::String::HexFormat::LowercaseAndPrefixed),
        size->to_nat_int_t());
}
