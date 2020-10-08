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
struct ParserValue;
struct ProcValue;
struct RangeValue;
struct RegexpValue;
struct StringValue;
struct SymbolValue;
struct TrueValue;
struct Value;
struct VoidPValue;
struct KernelModule;

void copy_hashmap(struct hashmap &, const struct hashmap &);

using MethodFnPtr = Value *(*)(Env *, Value *, ssize_t, Value **, Block *);

template <typename T>
struct Vector;

}
