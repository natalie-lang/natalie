#include <dlfcn.h>
#include <ffi.h>
#include <fstream>
#include <iostream>

#include "natalie.hpp"
#include "natalie/object_type.hpp"
#include "tm/owned_ptr.hpp"

using namespace Natalie;

Value init_ffi(Env *env, Value self) {
    return Value::nil();
}

static void *dlopen_wrapper(Env *env, const String &name) {
    void *handle = dlopen(name.c_str(), RTLD_LAZY);
    if (!handle) {
        auto error = [&name]() {
            // dlerror() clears the error message, so we need to set it again
            dlopen(name.c_str(), RTLD_LAZY);
            return nullptr;
        };
        // This might be a GNU ld script. In that case, mirror the behaviour of ruby-ffi and
        // read the file to find the actual shared object.
        auto const errmsg = dlerror();
        auto trail = strstr(errmsg, ": invalid ELF header");
        if (!trail) {
            static const auto so_ext = [&] {
                auto RbConfig = GlobalEnv::the()->Object()->const_fetch("RbConfig"_s).as_module();
                auto CONFIG = RbConfig->const_fetch("CONFIG"_s).as_hash_or_raise(env);
                auto SO_EXT = CONFIG->fetch(env, new StringObject { "SOEXT" }, nullptr, nullptr).as_string_or_raise(env);
                return String::format(".{}", SO_EXT->string());
            }();
            if (!name.begins_with('/') && !name.ends_with(so_ext)) {
                const auto new_name = String::format("{}{}", name, so_ext);
                return dlopen_wrapper(env, new_name);
            } else if (!name.begins_with('/') && !name.begins_with("lib")) {
                const auto new_name = String::format("lib{}", name);
                return dlopen_wrapper(env, new_name);
            }
            return error();
        }
        *trail = '\0';
        std::ifstream ldscript { errmsg, std::ios::in | std::ios::ate };
        if (!ldscript)
            return error();
        const auto size = ldscript.tellg();
        ldscript.seekg(0);
        char buf[static_cast<size_t>(size) + 1];
        if (!ldscript.read(buf, size))
            return error();
        ldscript.close();
        // Format: [not relevant] GROUP ( $FILENAME [not_relevant]
        auto pos = strstr(buf, "GROUP");
        if (!pos)
            return error();
        pos += 5; // skip "GROUP"
        while (*pos == ' ')
            pos++;
        if (*pos != '(')
            return error();
        pos++;
        while (*pos == ' ')
            pos++;
        if (*pos != '/')
            return error();
        auto endpos = strstr(pos, " ");
        if (!endpos)
            return error();
        *endpos = '\0';
        handle = dlopen(pos, RTLD_LAZY);
    }
    return handle;
}

static void *dlopen_wrapper(Env *env, Value name) {
    name.assert_type(env, Object::Type::String, "String");
    return dlopen_wrapper(env, name.as_string()->string());
}

Value FFI_Library_ffi_lib(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    auto name = args.at(0);
    void *handle = nullptr;
    if (name.is_array()) {
        for (auto name2 : *name.as_array()) {
            handle = dlopen_wrapper(env, name2);
            if (handle) {
                name = name2;
                break;
            }
        }
        if (!handle) {
            auto error = new StringObject;
            for (auto name2 : *name.as_array())
                error->append_sprintf("Could not open library '%s': %s.\n", name2.as_string()->string().c_str(), dlerror());
            error->chomp_in_place(env);
            env->raise("LoadError", error->string());
        }
    } else {
        handle = dlopen_wrapper(env, name);
        if (!handle)
            env->raise("LoadError", "Could not open library '{}': {}.", name.as_string()->c_str(), dlerror());
    }
    auto handle_ptr = new VoidPObject { handle, [](auto p) { dlclose(p->void_ptr()); } };
    auto libs = self->ivar_get(env, "@ffi_libs"_s);
    if (libs.is_nil())
        libs = self->ivar_set(env, "@ffi_libs"_s, new ArrayObject);
    auto DynamicLibrary = fetch_nested_const({ "FFI"_s, "DynamicLibrary"_s });
    auto lib = DynamicLibrary.send(env, "new"_s, { name, handle_ptr });
    libs.as_array()->push(lib);
    return Value::nil();
}

static ffi_type *get_ffi_type(Env *env, Value self, Value type) {
    type.assert_type(env, Object::Type::Symbol, "Symbol");
    auto type_sym = type.as_symbol();
    if (type_sym == "bool"_s) {
        return &ffi_type_ushort;
    } else if (type_sym == "char"_s) {
        return &ffi_type_uchar;
    } else if (type_sym == "double"_s) {
        return &ffi_type_double;
    } else if (type_sym == "int"_s) {
        return &ffi_type_sint;
    } else if (type_sym == "uint"_s) {
        return &ffi_type_uint;
    } else if (type_sym == "ulong"_s) {
        return &ffi_type_ulong;
    } else if (type_sym == "ulong_long"_s) {
        return &ffi_type_uint64;
    } else if (type_sym == "pointer"_s) {
        return &ffi_type_pointer;
    } else if (type_sym == "size_t"_s) {
        return &ffi_type_uint64;
    } else if (type_sym == "string"_s) {
        return &ffi_type_pointer;
    } else if (type_sym == "void"_s) {
        return &ffi_type_void;
    } else {
        auto typedefs = self->ivar_get(env, "@ffi_typedefs"_s);
        if (!typedefs.is_nil()) {
            if (typedefs.as_hash_or_raise(env)->has_key(env, type_sym)) {
                // FIXME: I'm pretty sure this is wrong, but I don't yet know what to return here.
                return &ffi_type_pointer;
            }
        }
        auto enums = self->ivar_get(env, "@enums"_s);
        if (!enums.is_nil()) {
            auto hash = enums.as_hash_or_raise(env);
            if (hash->has_key(env, type_sym)) {
                return &ffi_type_sint32;
            }
        }
        env->raise("TypeError", "unable to resolve type '{}'", type_sym->string());
    }
}

typedef union {
    void *vp;
    unsigned short us;
    unsigned char uc;
    int32_t s32;
    int sint;
    unsigned int uint;
    unsigned long ulong;
    uint64_t u64;
    double double_;
} FFI_Library_call_arg_slot;

static Value FFI_Library_fn_call_block(Env *env, Value self, Args &&args, Block *block) {
    Value cif_obj = env->outer()->var_get("cif", 0);
    auto cif = (ffi_cif *)cif_obj.as_void_p()->void_ptr();
    assert(cif);

    auto arg_types = env->outer()->var_get("arg_types", 1).as_array();
    auto return_type = env->outer()->var_get("return_type", 2).as_symbol();

    Value fn_obj = env->outer()->var_get("fn", 4);
    auto fn = (void (*)())fn_obj.as_void_p()->void_ptr();
    assert(fn);

    args.ensure_argc_is(env, cif->nargs);

    auto Pointer = fetch_nested_const({ "FFI"_s, "Pointer"_s }).as_class();

    auto bool_sym = "bool"_s;
    auto char_sym = "char"_s;
    auto double_sym = "double"_s;
    auto int_sym = "int"_s;
    auto uint_sym = "uint"_s;
    auto ulong_sym = "ulong"_s;
    auto ulong_long_sym = "ulong_long"_s;
    auto pointer_sym = "pointer"_s;
    auto size_t_sym = "size_t"_s;
    auto string_sym = "string"_s;
    auto void_sym = "void"_s;

    FFI_Library_call_arg_slot *arg_values = static_cast<FFI_Library_call_arg_slot *>(alloca(sizeof(FFI_Library_call_arg_slot) * cif->nargs));
    void **arg_pointers = static_cast<void **>(alloca(sizeof(void *) * cif->nargs));

    for (unsigned i = 0; i < cif->nargs; ++i) {
        auto val = args.at(i);
        auto type = arg_types->at(i);
        if (type == pointer_sym) {
            if (val.is_a(env, Pointer))
                arg_values[i].vp = (void *)val.send(env, "address"_s).integer_or_raise(env).to_nat_int_t();
            else if (val.is_string())
                arg_values[i].vp = (void *)val.as_string()->c_str();
            else if (val.is_nil())
                arg_values[i].vp = nullptr;
            else
                env->raise("ArgumentError", "Expected Pointer but got {} for arg {}", val.inspect_str(env), (int)i);
            arg_pointers[i] = &(arg_values[i].vp);
        } else if (type == bool_sym) {
            if (val == TrueObject::the())
                arg_values[i].us = 1;
            else if (val == FalseObject::the())
                arg_values[i].us = 0;
            else
                env->raise("TypeError", "wrong argument type (expected a boolean parameter)");
            arg_pointers[i] = &(arg_values[i].us);
        } else if (type == char_sym) {
            auto integer = val.integer_or_raise(env).to_nat_int_t();
            if (integer < 0 || integer > 256)
                arg_values[i].uc = 0;
            else
                arg_values[i].uc = integer;
            arg_pointers[i] = &(arg_values[i].uc);
        } else if (type == size_t_sym) {
            auto size = val.integer_or_raise(env).to_nat_int_t();
            if (size < 0 || (uint64_t)size > std::numeric_limits<uint64_t>::max())
                arg_values[i].u64 = 0;
            else
                arg_values[i].u64 = size;
            arg_pointers[i] = &(arg_values[i].u64);
        } else if (type == string_sym) {
            auto str = val.to_str(env);
            arg_values[i].vp = (void *)str->c_str();
            arg_pointers[i] = &(arg_values[i].vp);
        } else if (type == double_sym) {
            auto double_ = val.as_float_or_raise(env)->to_double();
            arg_values[i].double_ = double_;
            arg_pointers[i] = &(arg_values[i].double_);
        } else if (type == int_sym) {
            auto sint = IntegerMethods::convert_to_native_type<int>(env, val);
            arg_values[i].sint = sint;
            arg_pointers[i] = &(arg_values[i].sint);
        } else if (type == uint_sym) {
            auto uint = IntegerMethods::convert_to_native_type<unsigned int>(env, val);
            arg_values[i].uint = uint;
            arg_pointers[i] = &(arg_values[i].uint);
        } else if (type == ulong_sym) {
            auto ulong = IntegerMethods::convert_to_native_type<unsigned long>(env, val);
            arg_values[i].ulong = ulong;
            arg_pointers[i] = &(arg_values[i].ulong);
        } else if (type == ulong_long_sym) {
            auto ulong_long = IntegerMethods::convert_to_native_type<unsigned long long>(env, val);
            arg_values[i].u64 = ulong_long;
            arg_pointers[i] = &(arg_values[i].u64);
        } else {
            auto enums = self->ivar_get(env, "@enums"_s);
            if (!enums.is_nil() && enums.as_hash_or_raise(env)->has_key(env, type)) {
                auto enum_values = enums.as_hash()->ref(env, type);
                auto mapped_value = enum_values.send(env, "key"_s, { val });
                if (mapped_value.is_nil() && val.is_integer())
                    mapped_value = val;
                if (mapped_value.is_nil()) {
                    env->raise("ArgumentError", "invalid enum value, {}", val.inspect_str(env));
                } else {
                    const auto int_val = mapped_value.integer_or_raise(env).to_nat_int_t();
                    if (int_val < std::numeric_limits<int32_t>::min() || int_val > std::numeric_limits<int32_t>::max())
                        arg_values[i].s32 = 0;
                    else
                        arg_values[i].s32 = static_cast<int32_t>(int_val);
                    arg_pointers[i] = &(arg_values[i].s32);
                }
            } else {
                env->raise("StandardError", "I don't yet know how to handle argument type {} (arg {})", type.inspect_str(env), (int)i);
            }
        }
    }

    ffi_arg result;
    ffi_call(cif, fn, &result, arg_pointers);

    if (return_type == bool_sym) {
        return bool_object((uint64_t)result);
    } else if (return_type == char_sym) {
        return Value::integer(result);
    } else if (return_type == double_sym) {
        return new FloatObject { *reinterpret_cast<double *>(&result) };
    } else if (return_type == int_sym || return_type == uint_sym || return_type == ulong_sym) {
        return Value::integer(result);
    } else if (return_type == pointer_sym) {
        auto Pointer = fetch_nested_const({ "FFI"_s, "Pointer"_s }).as_class();
        auto pointer = Pointer->send(env, "new"_s, { "pointer"_s, Value::integer((nat_int_t)result) });
        return pointer;
    } else if (return_type == size_t_sym || return_type == ulong_long_sym) {
        assert((uint64_t)result <= std::numeric_limits<nat_int_t>::max());
        return Value::integer((nat_int_t)result);
    } else if (return_type == string_sym) {
        return new StringObject { reinterpret_cast<const char *>(result) };
    } else if (return_type == void_sym) {
        return Value::nil();
    } else {
        auto enums = self->ivar_get(env, "@enums"_s);
        if (!enums.is_nil()) {
            auto hash = enums.as_hash_or_raise(env);
            if (hash->has_key(env, return_type)) {
                hash = hash->ref(env, return_type).as_hash_or_raise(env);
                auto return_type_obj = Value::integer(static_cast<int32_t>(result));
                return hash->fetch(env, return_type_obj, return_type_obj, nullptr);
            }
        }
        env->raise("StandardError", "I don't yet know how to handle return type {}", return_type->string());
    }
}

Value FFI_Library_attach_function(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 3);
    auto name = args.at(0).to_symbol(env, Value::Conversion::Strict);
    auto arg_types = args.at(1);
    arg_types.assert_type(env, Object::Type::Array, "Array");
    auto return_type = args.at(2).to_symbol(env, Value::Conversion::Strict);
    // dbg("attach_function {v} {v} {v}", name, arg_types, return_type);

    auto arg_types_array = arg_types.as_array();
    auto arg_count = arg_types_array->size();

    auto ffi_args = new ffi_type *[arg_count];
    for (size_t i = 0; i < arg_count; ++i) {
        ffi_args[i] = get_ffi_type(env, self, arg_types_array->at(i));
    }
    auto ffi_args_obj = new VoidPObject {
        ffi_args,
        [](auto p) {
            auto ary = (ffi_type **)p->void_ptr();
            delete[] ary;
        }
    };

    auto libs = self->ivar_get(env, "@ffi_libs"_s);
    auto lib = libs.as_array()->first(); // what do we do if there is more than one?
    auto handle = lib->ivar_get(env, "@lib"_s).as_void_p()->void_ptr();

    dlerror(); // clear any previous error
    auto fn = dlsym(handle, name->string().c_str());
    auto error = dlerror();
    if (error || !fn) {
        auto NotFoundError = fetch_nested_const({ "FFI"_s, "NotFoundError"_s }).as_class();
        Value message;
        if (error)
            message = StringObject::format("Function '{}' not found in [{}]", name->string(), lib.send(env, "name"_s).as_string());
        else
            message = StringObject::format("Function '{}' not found in [{}] (unknown error)", name->string(), lib.send(env, "name"_s).as_string());
        auto exception = NotFoundError->send(env, "new"_s, { message }).as_exception();
        env->raise_exception(exception);
    }

    auto ffi_return_type = get_ffi_type(env, self, return_type);

    auto cif = new ffi_cif;
    auto status = ffi_prep_cif(
        cif,
        FFI_DEFAULT_ABI,
        arg_count,
        ffi_return_type,
        ffi_args);

    if (status != FFI_OK)
        env->raise("LoadError", "There was an error preparing the FFI call data structure: {}", (int)status);

    OwnedPtr<Env> block_env { new Env {} };
    block_env->var_set("cif", 0, true, new VoidPObject { cif, [](auto p) { delete (ffi_cif *)p->void_ptr(); } });
    block_env->var_set("arg_types", 1, true, arg_types_array);
    block_env->var_set("return_type", 2, true, return_type);
    block_env->var_set("ffi_args", 3, true, ffi_args_obj);
    block_env->var_set("fn", 4, true, new VoidPObject { fn });
    Block *block = new Block { std::move(block_env), self, FFI_Library_fn_call_block, 0 };
    Object::define_singleton_method(env, self, name, block);

    return Value::nil();
}

Value FFI_AbstractMemory_put_int8(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 2);

    auto offset = IntegerMethods::convert_to_native_type<size_t>(env, args.at(0));
    auto value = IntegerMethods::convert_to_native_type<int8_t>(env, args.at(1));

    auto address = (int8_t *)self->ivar_get(env, "@ptr"_s).as_void_p()->void_ptr();
    address[offset] = value;

    return self;
}

Value FFI_Pointer_address(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    auto address = self->ivar_get(env, "@ptr"_s).as_void_p()->void_ptr();
    return Value::integer((nat_int_t)address);
}

Value FFI_Pointer_read_string(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);

    auto address = (void *)self.send(env, "address"_s).integer_or_raise(env).to_nat_int_t();

    if (args.size() >= 1) {
        auto length = args.at(0).integer_or_raise(env).to_nat_int_t();
        if (length < 0 || (size_t)length > std::numeric_limits<size_t>::max())
            env->raise("ArgumentError", "length out of range");
        return new StringObject { (char *)address, (size_t)length, Encoding::ASCII_8BIT };
    }

    return new StringObject { (char *)address, Encoding::ASCII_8BIT };
}

Value FFI_Pointer_to_obj(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    return *(Value *)self.send(env, "address"_s).integer_or_raise(env).to_nat_int_t();
}

Value FFI_Pointer_write_string(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);

    auto str = args.at(0).as_string_or_raise(env);
    auto length = args.size() > 1 ? IntegerMethods::convert_to_native_type<size_t>(env, args.at(0)) : str->bytesize();

    auto address = (void *)self.send(env, "address"_s).integer_or_raise(env).to_nat_int_t();
    memcpy(address, str->c_str(), length);

    return self;
}

Value FFI_MemoryPointer_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 1, 3);

    size_t size = 0;
    if (args.at(0).is_symbol()) {
        auto sym = args.at(0).as_symbol();
        if (sym == "char"_s || sym == "uchar"_s || sym == "int8"_s || sym == "uint8"_s) {
            size = 1;
        } else if (sym == "short"_s || sym == "ushort"_s || sym == "int16"_s || sym == "uint16"_s) {
            size = 2;
        } else if (sym == "int32"_s || sym == "uint32"_s || sym == "float"_s) {
            size = 4;
        } else if (sym == "int64"_s || sym == "uint64"_s || sym == "long_long"_s || sym == "ulong_long"_s || sym == "double"_s) {
            size = 8;
        } else if (sym == "int"_s) {
            size = sizeof(signed int);
        } else if (sym == "uint"_s) {
            size = sizeof(unsigned int);
        } else if (sym == "long"_s) {
            size = sizeof(long int);
        } else if (sym == "ulong"_s) {
            size = sizeof(unsigned long int);
        } else if (sym == "pointer"_s) {
            size = sizeof(void *);
        } else {
            env->raise("TypeError", "unknown size argument for FFI#initialize: {}", args.at(0).inspect_str(env));
        }
    } else {
        size = IntegerMethods::convert_to_native_type<size_t>(env, args.at(0));
    }

    size_t count = 1;
    if (args.size() > 1)
        count = IntegerMethods::convert_to_native_type<size_t>(env, args.at(1));

    self->ivar_set(env, "@size"_s, Value::integer(size * count));

    bool clear = args.size() > 2 && args.at(2).is_truthy();

    void *address = nullptr;
    if (clear)
        address = calloc(count, size);
    else
        address = malloc(count * size);
    auto address_obj = Value::integer((nat_int_t)address);

    super(env, self, { "pointer"_s, address_obj }, nullptr);

    self->ivar_set(env, "@type_size"_s, Value::integer(size));

    auto ptr_obj = self->ivar_get(env, "@ptr"_s);
    ptr_obj->ivar_set(env, "@autorelease"_s, TrueObject::the());

    if (block)
        block->run(env, Args({ self }), nullptr);

    return Value::nil();
}

Value FFI_Pointer_initialize(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);

    Value type, address;
    int type_size = 1;

    if (args.size() == 2) {
        type = args.at(0);
        address = args.at(1);
        if (type == "pointer"_s)
            type_size = sizeof(void *);
        else
            NAT_NOT_YET_IMPLEMENTED("I don't yet know how to handle type %s in FFI::Pointer.new", type.inspect_str(env).c_str());
    } else {
        type = Value::nil();
        address = args.at(0);
    }

    auto ptr_obj = new VoidPObject {
        (void *)address.integer_or_raise(env).to_nat_int_t(),
        [](auto p) {
            Env e;
            if (p->void_ptr() && p->ivar_get(&e, "@autorelease"_s).is_truthy()) {
                free(p->void_ptr());
                p->set_void_ptr(nullptr);
            }
        }
    };
    ptr_obj->ivar_set(env, "@autorelease"_s, FalseObject::the());

    self->ivar_set(env, "@ptr"_s, ptr_obj);
    self->ivar_set(env, "@type_size"_s, Value::integer(type_size));

    return Value::nil();
}

Value FFI_Pointer_free(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    auto ptr_obj = self->ivar_get(env, "@ptr"_s).as_void_p();
    auto address = ptr_obj->void_ptr();

    ptr_obj->set_void_ptr(nullptr);
    free(address);

    return Value::nil();
}

Value FFI_MemoryPointer_inspect(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    auto ptr = (void *)self.send(env, "address"_s).integer_or_raise(env).to_nat_int_t();
    auto size = self->ivar_get(env, "@size"_s).integer();
    return StringObject::format(
        "#<FFI::MemoryPointer address={} size={}>",
        TM::String::hex((uintptr_t)ptr, TM::String::HexFormat::LowercaseAndPrefixed),
        size.to_nat_int_t());
}

Value FFI_Pointer_is_autorelease(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    auto ptr_obj = self->ivar_get(env, "@ptr"_s);
    return ptr_obj->ivar_get(env, "@autorelease"_s);
}

Value FFI_Pointer_set_autorelease(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);

    auto ptr_obj = self->ivar_get(env, "@ptr"_s);
    return ptr_obj->ivar_set(env, "@autorelease"_s, bool_object(args.at(0).is_truthy()));
}

Value Object_to_ptr(Env *env, Value self, Args &&, Block *) {
    auto address = Value::integer((nat_int_t)self.object());
    auto Pointer = fetch_nested_const({ "FFI"_s, "Pointer"_s });
    return Pointer.send(env, "new"_s, { address });
}
