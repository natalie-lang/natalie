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
struct FiberValue;
struct FileValue;
struct FloatValue;
struct GlobalEnv;
struct HashKey;
struct HashVal;
struct HashValue;
struct IntegerValue;
struct IoValue;
struct KernelModule;
struct Lexer;
struct MatchDataValue;
struct Method;
struct ModuleValue;
struct NilValue;
struct Node;
struct Parser;
struct ParserValue;
struct ProcValue;
struct RangeValue;
struct RegexpValue;
struct SexpValue;
struct StringValue;
struct SymbolValue;
struct Token;
struct TrueValue;
struct Value;
struct VoidPValue;

void copy_hashmap(struct hashmap &, const struct hashmap &);

using ValuePtr = Value *;
using MethodFnPtr = ValuePtr (*)(Env *, ValuePtr, size_t, ValuePtr *, Block *);

template <typename T>
struct Vector;

}
