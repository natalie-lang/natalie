#pragma once

#include <unistd.h>

namespace Natalie {

struct ArrayValue;
struct Block;
struct ClassValue;
struct EncodingValue;
struct Env;
struct EnvValue;
struct ExceptionValue;
struct FalseValue;
struct FileValue;
struct FloatValue;
struct GlobalEnv;
struct HashKey;
struct HashVal;
struct HashValue;
struct IntegerValue;
struct IoValue;
struct MatchDataValue;
struct Method;
struct ModuleValue;
struct NilValue;
struct ProcValue;
struct RangeValue;
struct RegexpValue;
struct StringValue;
struct SymbolValue;
struct TrueValue;
struct Value;
struct VoidPValue;
struct KernelModule;

using ID = size_t;
ID intern(Env *, const char *);
SymbolValue *intern_and_return_symbol(Env *, ID, const char *);

void copy_hashmap(struct hashmap &, const struct hashmap &);
size_t hashmap_identity(const void *);
int hashmap_compare_identity(const void *, const void *);

using MethodFnPtr = Value *(*)(Env *, Value *, ssize_t, Value **, Block *);

template <typename T>
struct Vector;

}
