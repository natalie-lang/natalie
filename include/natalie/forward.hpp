#pragma once

#include <unistd.h>

namespace Natalie {

namespace Enumerator {
    class ArithmeticSequenceObject;
};
namespace Thread {
    namespace Backtrace {
        class LocationObject;
    }
    class MutexObject;
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
class FalseMethods;
class FiberObject;
class FileObject;
class FileStatObject;
class FloatObject;
class GlobalEnv;
class HashObject;
class IoObject;
class KernelModule;
class MatchDataObject;
class Method;
class MethodObject;
class ModuleObject;
class NatalieProfiler;
class NatalieProfilerEvent;
class NumberParser;
class ParserObject;
class ProcObject;
class RandomObject;
class RangeObject;
class RationalObject;
class RegexpObject;
class SignalModule;
class StringObject;
class SymbolObject;
class ThreadObject;
class ThreadGroupObject;
class TimeObject;
class TrueMethods;
class Object;
class UnboundMethodObject;
class VoidPObject;

class Value;

class Args;

class Integer;

using MethodFnPtr = Value (*)(Env *, Value, Args &&, Block *);

}
