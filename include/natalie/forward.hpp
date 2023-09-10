#pragma once

#include <unistd.h>

namespace Natalie {

namespace Enumerator {
    class ArithmeticSequenceObject;
};
class ArrayObject;
class Backtrace;
class BindingObject;
class Block;
class ClassObject;
class ComplexObject;
class DirObject;
class EncodingObject;
class Env;
class EnvObject;
class ExceptionObject;
class FalseObject;
class FiberObject;
class FileObject;
class FileStatObject;
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
class SignalModule;
class StringObject;
class SymbolObject;
class TimeObject;
class TrueObject;
class Object;
class UnboundMethodObject;
class VoidPObject;

class Value;

class Args;

using MethodFnPtr = Value (*)(Env *, Value, Args, Block *);

}
