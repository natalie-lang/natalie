#pragma once

#include <unistd.h>

namespace Natalie {

class ArrayValue;
class Block;
class ClassValue;
class EncodingValue;
class Env;
class EnvValue;
class ExceptionValue;
class FalseValue;
class FiberValue;
class FileValue;
class FloatValue;
class GlobalEnv;
class HashKey;
class HashVal;
class HashValue;
class IntegerValue;
class IoValue;
class KernelModule;
class Lexer;
class MatchDataValue;
class Method;
class MethodValue;
class ModuleValue;
class NilValue;
class Node;
class Parser;
class ParserValue;
class ProcValue;
class RangeValue;
class RegexpValue;
class SexpValue;
class String;
class StringValue;
class SymbolValue;
class Token;
class TrueValue;
class Value;
class VoidPValue;

class ValuePtr;

using MethodFnPtr = ValuePtr (*)(Env *, ValuePtr, size_t, ValuePtr *, Block *);

}
