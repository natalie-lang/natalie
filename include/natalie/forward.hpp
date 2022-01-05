#pragma once

#include <unistd.h>

namespace Natalie {

class ArrayObject;
class Block;
class ClassObject;
class EncodingObject;
class Env;
class EnvObject;
class ExceptionObject;
class FalseObject;
class FiberObject;
class FileObject;
class FloatObject;
class GlobalEnv;
class HashKey;
class HashVal;
class HashObject;
class IntegerObject;
class IoObject;
class KernelModule;
class Lexer;
class MatchDataObject;
class Method;
class MethodObject;
class ModuleObject;
class NilObject;
class Node;
class Parser;
class ParserObject;
class ProcObject;
class RandomObject;
class RangeObject;
class RegexpObject;
class SexpObject;
class String;
class StringObject;
class SymbolObject;
class Token;
class TrueObject;
class Object;
class UnboundMethodObject;
class VoidPObject;

class Value;

using MethodFnPtr = Value (*)(Env *, Value, size_t, Value *, Block *);

}
