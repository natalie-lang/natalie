#pragma once

namespace Natalie {

struct ArrayValue;
struct Block;
struct ClassValue;
struct EncodingValue;
struct Env;
struct ExceptionValue;
struct FalseValue;
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

template <typename T>
struct Vector;

typedef HashKey HashIter;

}
