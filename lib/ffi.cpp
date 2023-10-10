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

static Value FFI_Library_fn_call_block(Env *env, Value self, Args args, Block *block) {
    Value cif_obj = env->outer()->var_get("cif", 0);
    auto cif = (ffi_cif *)cif_obj->as_void_p()->void_ptr();
    assert(cif);

    Value fn_obj = env->outer()->var_get("fn", 1);
    auto fn = (void (*)())fn_obj->as_void_p()->void_ptr();
    assert(fn);

    void *result = nullptr;
    void *values[args.size()];
    // TODO: set values

    ffi_call(cif, fn, &result, values);

    if (cif->rtype == &ffi_type_pointer) {
        auto Pointer = fetch_nested_const({ "FFI"_s, "Pointer"_s })->as_class();
        auto pointer = Pointer->send(env, "new"_s);
        pointer->ivar_set(env, "@address"_s, new VoidPObject { result });
        return pointer;
    } else if (cif->rtype == &ffi_type_uint64) {
        assert((uint64_t)result <= std::numeric_limits<nat_int_t>::max());
        return Value::integer((nat_int_t)result);
    } else {
        printf("rtype = %p\n", cif->rtype);
        printf("ffi_type_void = %p\n", &ffi_type_void);
        env->raise("StandardError", "I don't yet know how to handle return type {}", (long long)cif->rtype);
    }
}

Value FFI_Library_attach_function(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 3);
    auto name = args.at(0)->to_symbol(env, Object::Conversion::Strict);
    auto arg_types = args.at(1);
    arg_types->assert_type(env, Object::Type::Array, "Array");
    auto return_type = args.at(2)->to_symbol(env, Object::Conversion::Strict);

    auto arg_types_array = arg_types->as_array();
    auto arg_count = arg_types_array->size();

    ffi_type *ffi_args[arg_count];
    for (size_t i = 0; i < arg_count; ++i) {
        ffi_args[i] = get_ffi_type(env, arg_types_array->at(i));
    }

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
    block_env->var_set("fn", 1, true, new VoidPObject { fn });
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
