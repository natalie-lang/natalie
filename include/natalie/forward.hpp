#pragma once

#include <unistd.h>

namespace Natalie {

class ArrayObject;
class Backtrace;
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
class MatchDataObject;
class Method;
class MethodObject;
class ModuleObject;
class NatalieProfiler;
class NatalieProfilerEvent;
class NilObject;
class ParserObject;
class ProcObject;
class RandomObject;
class RangeObject;
class RationalObject;
class RegexpObject;
class SexpObject;
class StringObject;
class SymbolObject;
class TrueObject;
class Object;
class UnboundMethodObject;
class VoidPObject;

class Value;

struct Args;

using MethodFnPtr = Value (*)(Env *, Value, Args, Block *);

}
