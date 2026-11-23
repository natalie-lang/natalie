#include "natalie/llvm_runtime.hpp"

using namespace Natalie;

extern "C" uint64_t nat_llvm_integer(const char *digits) {
    return nat_llvm_pack(Value::integer(TM::String(digits)));
}

extern "C" uint64_t nat_llvm_float(double number) {
    return nat_llvm_pack(FloatObject::create(number));
}

extern "C" uint64_t nat_llvm_string(const char *bytes, uint64_t size, int status) {
    auto string = StringObject::create(bytes, size, EncodingObject::get(Encoding::UTF_8));
    if (status == 1)
        string->set_chilled(StringObject::Chilled::String);
    else if (status == 2)
        string->freeze();
    return nat_llvm_pack(string);
}

extern "C" uint64_t nat_llvm_symbol(const char *name, uint64_t size) {
    return nat_llvm_pack(SymbolObject::intern(name, size));
}

extern "C" uint64_t nat_llvm_array(uint64_t count, const uint64_t *items) {
    auto array = ArrayObject::create();
    for (uint64_t i = 0; i < count; ++i)
        array->push(nat_llvm_unpack(items[i]));
    return nat_llvm_pack(array);
}

extern "C" uint64_t nat_llvm_object_class() {
    return nat_llvm_pack(GlobalEnv::the()->Object());
}

extern "C" uint64_t nat_llvm_const(Env *env, uint64_t namespace_bits, uint64_t self_bits, const char *name, int strict, int raise) {
    auto search = strict ? Object::ConstLookupSearchMode::Strict : Object::ConstLookupSearchMode::NotStrict;
    auto failure = raise ? Object::ConstLookupFailureMode::Raise : Object::ConstLookupFailureMode::ConstMissing;
    return nat_llvm_pack(Object::const_find_with_autoload(env, nat_llvm_unpack(namespace_bits), nat_llvm_unpack(self_bits), SymbolObject::intern(name), search, failure).value());
}

extern "C" uint64_t nat_llvm_send(Env *env, uint64_t self_bits, uint64_t receiver_bits, const char *name, uint64_t argc, const uint64_t *argv, int public_only, int keywords, const char *file, uint64_t line) {
    env->set_file(file);
    env->set_line(line);
    TM::Vector<Value> values;
    for (uint64_t i = 0; i < argc; ++i)
        values.push(nat_llvm_unpack(argv[i]));
    Args args(values, keywords);
    auto receiver = nat_llvm_unpack(receiver_bits);
    auto symbol = SymbolObject::intern(name);
    if (public_only)
        return nat_llvm_pack(receiver.public_send(env, symbol, std::move(args), nullptr, nat_llvm_unpack(self_bits)));
    return nat_llvm_pack(receiver.send(env, symbol, std::move(args), nullptr));
}

extern "C" void nat_llvm_set_zero(Env *env, const char *path) {
    env->global_set("$0"_s, StringObject::create(path));
}

extern "C" void nat_llvm_warn(Env *env, uint64_t self_bits, const char *message, int verbose) {
    if (verbose && !GlobalEnv::the()->is_verbose())
        return;
    nat_llvm_unpack(self_bits).send(env, "warn"_s, { StringObject::create(message) });
}
